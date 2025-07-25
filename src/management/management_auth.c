#include "management_auth.h"

#include "../auth/auth_parser.h"
#include "../auth/auth.h"
#include "../management/management.h"
#include <sys/socket.h>
#include <stdio.h>

void management_auth_init(unsigned int state, struct selector_key *key) {
    management_client *data = ATTACHMENT_MANAGEMENT(key);
    auth_parser_init(&data->management_parser.auth_parser);
}

unsigned management_auth_read(struct selector_key *key) {
    management_client *data = ATTACHMENT_MANAGEMENT(key);
    struct auth_parser *p = &data->management_parser.auth_parser;

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

unsigned management_auth_write(struct selector_key *key) {
    management_client *data = ATTACHMENT_MANAGEMENT(key);

    size_t write_limit;    // Maximum bytes to write in this operation
    ssize_t write_count;   // Total bytes written in this operation
    uint8_t *write_buffer = buffer_read_ptr(&data->response_buffer, &write_limit);

    write_count = send(key->fd, write_buffer, write_limit, MSG_NOSIGNAL);
    if (write_count <= 0) {
        return MANAGEMENT_ERROR;
    }

    buffer_read_adv(&data->response_buffer, write_count);
    if (buffer_can_read(&data->response_buffer)) {
        return MANAGEMENT_ERROR;
    }

    if (auth_parser_has_error(&data->management_parser.auth_parser)) {
        return MANAGEMENT_ERROR;
    }

    if (!auth_parser_is_authenticated(&data->management_parser.auth_parser)) {
        return MANAGEMENT_ERROR;
    }

    if (selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) {
        return MANAGEMENT_ERROR;
    }
    return MANAGEMENT_REQUEST_READ;
}