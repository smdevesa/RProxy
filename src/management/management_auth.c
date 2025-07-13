#include "management_auth.h"

#include "../auth/auth_parser.h"
#include "../auth/auth.h"
#include "../buffer.h"
#include "../users.h"
#include "../selector.h"
#include "../management/management.h"
#include <sys/socket.h>


void management_auth_init(unsigned int state, struct selector_key *key) {
    struct management_data *data = ATTACHMENT(key);
    auth_parser_init(&data->auth_parser);
}

unsigned management_auth_read(struct selector_key *key) {
    management_client *data = ATTACHMENT(key);
    struct auth_parser *p = &data->auth_parser;

    size_t read_limit;      // Maximum bytes to read in this operation
    ssize_t read_count;     // Total bytes read in this operation
    uint8_t *read_buffer = buffer_write_ptr(&data->request_buffer, &read_limit);
    read_count = recv(key->fd, read_buffer, read_limit, 0);

    if (read_count <= 0) {
        return MANAGEMENT_ERROR;
    }

    buffer_write_adv(&data->request_buffer, read_count);
    auth_parser_parse(p, &data->request_buffer);

    if (auth_parser_is_done(p)) {
        if (auth_parser_has_error(p)) {
            return MANAGEMENT_ERROR;
        }

        try_to_authenticate(p);
        data->is_admin = p->is_admin;
        if (selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS || !auth_parser_build_response(p, &data->response_buffer)) {
            return MANAGEMENT_ERROR;
        }
        return MANAGEMENT_AUTH_WRITE;
    }

    return MANAGEMENT_AUTH_READ;
}

unsigned auth_write(struct selector_key *key) {
    management_client *data = ATTACHMENT(key);

    size_t write_limit;    // Maximum bytes to write in this operation
    ssize_t write_count;   // Total bytes written in this operation
    uint8_t *write_buffer = buffer_read_ptr(&data->request_buffer, &write_limit);

    write_count = send(key->fd, write_buffer, write_limit, MSG_NOSIGNAL);
    if (write_count <= 0) {
        return MANAGEMENT_ERROR;
    }

    buffer_read_adv(&data->request_buffer, write_count);
    if (buffer_can_read(&data->request_buffer)) {
        return MANAGEMENT_ERROR;
    }

    if (auth_parser_has_error(&data->auth_parser)) {
        return MANAGEMENT_ERROR;
    }

    if (!auth_parser_is_authenticated(&data->auth_parser)) {
        return MANAGEMENT_ERROR;
    }

    if (selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) {
        return MANAGEMENT_ERROR;
    }
    return MANAGEMENT_AUTH_READ;
}