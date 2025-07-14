#include "management_command.h"
#include "management.h"
#include "management_command_parser.h"
#include <sys/socket.h>

static bool management_process_command(struct management_command_parser *parser, struct buffer * buffer, int fd);

typedef int (*management_command_handler)(struct management_command_parser *parser, struct buffer *response_buffer, int client_fd);

static int users_command_handler(struct management_command_parser *parser, struct buffer *response_buffer, int client_fd);
static int add_user_command_handler(struct management_command_parser *parser, struct buffer *response_buffer, int client_fd);
static int delete_users_command_handler(struct management_command_parser *parser, struct buffer *response_buffer, int client_fd);
static int change_password_command_handler(struct management_command_parser *parser, struct buffer *response_buffer, int client_fd);
static int stats_command_handler(struct management_command_parser *parser, struct buffer *response_buffer, int client_fd);
static int change_role_command_handler(struct management_command_parser *parser, struct buffer *response_buffer, int client_fd);

static management_command_handler command_handlers[] = {
    users_command_handler,
    add_user_command_handler,
    delete_users_command_handler,
    change_password_command_handler,
    stats_command_handler,
    change_role_command_handler
};

void management_command_read_init(struct selector_key *key) {
    struct management_client *data = ATTACHMENT(key);
    management_command_parser_init(&data->management_parser.request_parser);
}

int management_command_read(struct selector_key *key) {
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
    if(management_command_parser_is_done(&data->management_parser.request_parser)) {
        if (selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS ||
        !management_process_command(&data->management_parser.request_parser, &data->response_buffer, data->client_fd)) {
            return MANAGEMENT_ERROR;
        }
        data->command = data->management_parser.request_parser.command;
        return MANAGEMENT_REQUEST_WRITE;
    }
    return MANAGEMENT_REQUEST_READ;
}

int management_command_write(struct selector_key *key) {
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

static bool management_process_command(struct management_command_parser *parser, struct buffer * buffer, int fd) {
    if(parser->command <= MANAGEMENT_COMMAND_MAX) {
        int response = command_handlers[parser->command](parser, buffer, fd);
        return management_command_parser_build_response(parser, buffer, response);
    }
    return false;
}