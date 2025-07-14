#include "management_command.h"
#include "management.h"
#include "management_command_parser.h"
#include "../metrics/metrics.h"
#include "../users.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

static bool management_process_command(struct management_command_parser *parser, struct buffer * buffer, bool is_admin);

typedef bool (*management_command_handler)(struct management_command_parser *parser, struct buffer *response_buffer);

static bool users_command_handler(struct management_command_parser *parser, struct buffer *response_buffer);
static bool add_user_command_handler(struct management_command_parser *parser, struct buffer *response_buffer);
static bool delete_users_command_handler(struct management_command_parser *parser, struct buffer *response_buffer);
static bool change_password_command_handler(struct management_command_parser *parser, struct buffer *response_buffer);
static bool stats_command_handler(struct management_command_parser *parser, struct buffer *response_buffer);
static bool change_role_command_handler(struct management_command_parser *parser, struct buffer *response_buffer);

static management_command_handler command_handlers[] = {
    (management_command_handler) users_command_handler,
    (management_command_handler) add_user_command_handler,
    (management_command_handler) delete_users_command_handler,
    (management_command_handler) change_password_command_handler,
    (management_command_handler) stats_command_handler,
    (management_command_handler) change_role_command_handler
};

static bool needs_admin_privileges[] = {
    false, // MANAGEMENT_COMMAND_USERS
    true,  // MANAGEMENT_COMMAND_ADD_USER
    true,  // MANAGEMENT_COMMAND_DELETE_USERS
    true,  // MANAGEMENT_COMMAND_CHANGE_PASSWORD
    false, // MANAGEMENT_COMMAND_STATS
    true   // MANAGEMENT_COMMAND_CHANGE_ROLE
};

void management_command_read_init(unsigned state, struct selector_key *key) {
    struct management_client *data = ATTACHMENT(key);
    management_command_parser_init(&data->management_parser.request_parser);
}

unsigned management_command_read(struct selector_key *key) {
    struct management_client *data = ATTACHMENT(key);

    size_t read_limit;
    ssize_t read_count;
    uint8_t *buf;

    buf = buffer_write_ptr(&data->request_buffer, &read_limit);
    read_count = recv(key->fd, buf, read_limit, 0);
    if (read_count <= 0) {
        return MANAGEMENT_ERROR; // Error or connection closed
    }

    buffer_write_adv(&data->request_buffer, read_count);
    management_command_parser_parse(&data->management_parser.request_parser, &data->request_buffer);

    if (management_command_parser_is_done(&data->management_parser.request_parser)) {
        if (management_command_parser_has_error(&data->management_parser.request_parser)) {
            return MANAGEMENT_ERROR;
        }

        if (!management_process_command(&data->management_parser.request_parser, &data->response_buffer,data->is_admin)) {
            return MANAGEMENT_ERROR;
        }
        if (selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS)
            return MANAGEMENT_ERROR;

        data->command = data->management_parser.request_parser.command;
        return MANAGEMENT_REQUEST_WRITE;
    }
    return MANAGEMENT_REQUEST_READ;
}

unsigned management_command_write(struct selector_key *key) {
    struct management_client *data = ATTACHMENT(key);

    size_t write_limit;
    ssize_t write_count;
    uint8_t *buf;

    buf = buffer_read_ptr(&data->response_buffer, &write_limit);
    write_count = send(key->fd, buf, write_limit, MSG_NOSIGNAL);
    if(write_count <= 0) {
        return MANAGEMENT_ERROR; // Error or connection closed
    }
    buffer_read_adv(&data->response_buffer, write_count);
    if(buffer_can_read(&data->response_buffer)) {
        return MANAGEMENT_REQUEST_WRITE;
    }
    if(management_command_parser_has_error(&data->management_parser.request_parser)) {
        return MANAGEMENT_ERROR;
    }
    return MANAGEMENT_CLOSED;
}

static bool add_user_command_handler(struct management_command_parser *parser,
                                     struct buffer *response_buffer) {

    if (parser->read_args != 2) {
        return management_command_parser_build_response(parser, response_buffer,
                                                        MANAGEMENT_INVALID_ARGUMENTS,
                                                        "add_user: invalid argument count, expected 2");;
    }

    const char *username = (const char *)parser->args[0];
    const char *password = (const char *)parser->args[1];

    if (exists_user(username)) {
        return management_command_parser_build_response(parser, response_buffer,
                                                 MANAGEMENT_USER_ALREADY_EXISTS,
                                                 "add_user: user already exists");
    }

    if (create_user(username, password, false)) {
        return management_command_parser_build_response(parser, response_buffer,
                                                 MANAGEMENT_SUCCESS,
                                                 "add_user: user added successfully");
    }

    return management_command_parser_build_response(parser, response_buffer,
                                             MANAGEMENT_SERVER_ERROR,
                                             "add_user: failed to add user");
}

static bool delete_users_command_handler(struct management_command_parser *parser,
                                         struct buffer *response_buffer) {
    if (parser->read_args != 1) {
        return management_command_parser_build_response(parser, response_buffer,
                                                 MANAGEMENT_INVALID_ARGUMENTS,
                                                 "delete_users: invalid argument count, expected 1");
    }

    const char *username = (const char *)parser->args[0];
    if (!exists_user(username)) {
        return management_command_parser_build_response(parser, response_buffer,
                                                 MANAGEMENT_USER_NOT_FOUND,
                                                 "delete_users: user not found");
    }
    if (delete_user(username)) {
        return management_command_parser_build_response(parser, response_buffer,
                                                 MANAGEMENT_SUCCESS,
                                                 "delete_users: user deleted successfully");
    }
    return management_command_parser_build_response(parser, response_buffer,
                                             MANAGEMENT_SERVER_ERROR,
                                             "delete_users: failed to delete user");
}

static bool users_command_handler(struct management_command_parser *parser,
                                  struct buffer *response_buffer) {
    if(parser->read_args != 0) {
        return management_command_parser_build_response(parser, response_buffer,
                                                 MANAGEMENT_INVALID_ARGUMENTS,
                                                 "users: invalid argument count, expected 0");
    }

    int user_count = users_get_count();

    char header[64];
    int header_len = snprintf(header, sizeof(header), "users count: %d\n", user_count);

    if (!management_command_parser_build_response(parser, response_buffer,
                                                  MANAGEMENT_SUCCESS, NULL)) {
        return false;
                                                  }

    size_t available;
    uint8_t *ptr = buffer_write_ptr(response_buffer, &available);

    if ((size_t)header_len > available) {
        return false;
    }
    memcpy(ptr, header, header_len);
    buffer_write_adv(response_buffer, header_len);

    ptr = buffer_write_ptr(response_buffer, &available);

    size_t written = users_dump_usernames(ptr, available);

    if (written == 0) {
        return false;
    }
    buffer_write_adv(response_buffer, written);

    return true;
}

static bool change_password_command_handler(struct management_command_parser *parser,
                                            struct buffer *response_buffer) {
    if (parser->read_args != 2) {
        return management_command_parser_build_response(parser, response_buffer,
                                                 MANAGEMENT_INVALID_ARGUMENTS,
                                                 "change_password: invalid argument count, expected 2");
    }

    const char *username = (const char *)parser->args[0];
    const char *new_password = (const char *)parser->args[1];

    if (!exists_user(username)) {
        return management_command_parser_build_response(parser, response_buffer,
                                                 MANAGEMENT_USER_NOT_FOUND,
                                                 "change_password: user not found");
    }

    if (!change_user_password(username, new_password)) {
        return management_command_parser_build_response(parser, response_buffer,
                                             MANAGEMENT_SERVER_ERROR,
                                             "change_password: failed to change password");
    }

    return management_command_parser_build_response(parser, response_buffer,
                                             MANAGEMENT_SUCCESS,
                                             "change_password: password changed successfully");
}

static bool change_role_command_handler(struct management_command_parser *parser,
                                            struct buffer *response_buffer) {
    if (parser->read_args != 2) {
        return management_command_parser_build_response(parser, response_buffer,
                                                 MANAGEMENT_INVALID_ARGUMENTS,
                                                 "change_role: invalid argument count, expected 2");
    }

    const char *username = (const char *)parser->args[0];
    const char *role_str = (const char *)parser->args[1];

    if (!exists_user(username)) {
        return management_command_parser_build_response(parser, response_buffer,
                                                 MANAGEMENT_USER_NOT_FOUND,
                                                 "change_role: user not found");
    }

    bool is_admin = strcmp(role_str, "admin") == 0;
    if (!is_admin && strcmp(role_str, "user") != 0) {
        return management_command_parser_build_response(parser, response_buffer,
                                                 MANAGEMENT_INVALID_ROLE,
                                                 "change_role: invalid role, expected 'admin' or 'user'");
    }

    if (!change_user_role(username, is_admin)) {
        return management_command_parser_build_response(parser, response_buffer,
                                             MANAGEMENT_SERVER_ERROR,
                                             "change_role: failed to change role");
    }

    return management_command_parser_build_response(parser, response_buffer,
                                             MANAGEMENT_SUCCESS,
                                             "change_role: role changed successfully");
}

static bool management_process_command(struct management_command_parser *parser,
                                       struct buffer *resp, bool is_admin) {
    {
        size_t n = sizeof command_handlers / sizeof command_handlers[0];
        if ((size_t) parser->command >= n)          /* enum fuera de rango */
            return false;

        if (needs_admin_privileges[parser->command] && !is_admin) {
            return management_command_parser_build_response(parser, resp,
                                                            MANAGEMENT_FORBIDDEN,
                                                            "command requires admin privileges");
        }

        return command_handlers[parser->command](parser, resp);
    }
}

static bool stats_command_handler(struct management_command_parser *parser, struct buffer *response_buffer) {
    printf("Calling stats command handler\n");
    if (parser->read_args != 0) {
        return management_command_parser_build_response(parser, response_buffer,
                                                 MANAGEMENT_INVALID_ARGUMENTS,
                                                 "stats: invalid argument count, expected 0");
    }

    metrics_data_t metrics_data;
    printf("About to get metrics data\n");
    get_metrics_data(&metrics_data);

    char response[256];
    int response_len = snprintf(response, sizeof(response),
                                "Current connections: %zu\n"
                                "Total connections: %zu\n"
                                "Bytes sent: %zu\n"
                                "Bytes received: %zu\n",
                                metrics_data.current_connections,
                                metrics_data.total_connections,
                                metrics_data.bytes_sent,
                                metrics_data.bytes_received);

    if (!management_command_parser_build_response(parser, response_buffer,
                                                  MANAGEMENT_SUCCESS, NULL)) {
        return false;
    }

    size_t available;
    uint8_t *ptr = buffer_write_ptr(response_buffer, &available);
    if ((size_t)response_len > available) return false;

    memcpy(ptr, response, response_len);
    buffer_write_adv(response_buffer, response_len);

    return true;
}