#include "auth.h"

#include <stdio.h>

#include "../socks5/socks5.h"

void auth_read_init(unsigned state, struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    auth_parser_init(&data->client.auth_parser);
}

unsigned auth_read(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    struct auth_parser *p = &data->client.auth_parser;

    size_t read_limit;      // Maximum bytes to read in this operation
    ssize_t read_count;     // Total bytes read in this operation
    uint8_t *read_buffer = buffer_write_ptr(&data->client_buffer, &read_limit);
    read_count = recv(key->fd, read_buffer, read_limit, 0);
    if (read_count <= 0) {
        return ERROR;
    }

    buffer_write_adv(&data->client_buffer, read_count);
    auth_parser_parse(p, &data->client_buffer);

    if(auth_parser_is_done(p)) {
        if (auth_parser_has_error(p)) {
            return ERROR;
        }

        try_to_authenticate(p);
        if (selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS || !auth_parser_build_response(p, &data->origin_buffer)) {
            return ERROR;
        }
        return AUTH_WRITE;
    }

    return AUTH_READ;
}

unsigned auth_write(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);

    size_t write_limit;     // Maximum bytes to write in this operation
    ssize_t write_count;    // Total bytes written in this operation
    uint8_t *write_buffer = buffer_read_ptr(&data->origin_buffer, &write_limit);
    write_count = send(key->fd, write_buffer, write_limit, MSG_NOSIGNAL);
    if (write_count <= 0) {
        return ERROR;
    }

    buffer_read_adv(&data->origin_buffer, write_count);
    if (buffer_can_read(&data->origin_buffer)) {
        // If there is still data to write, continue writing
        return AUTH_WRITE;
    }

    if (auth_parser_has_error(&data->client.auth_parser)) return ERROR;
    if (!auth_parser_is_authenticated(&data->client.auth_parser)) return ERROR;
    if (selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) return ERROR;

    return REQUEST_READ;
}
