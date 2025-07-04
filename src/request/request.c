////
//// Created by Tizifuchi12 on 4/7/2025.
////
//
#include "request.h"

#include "../request/request_parser.h"
#include "../socks5/socks5.h"
#include "../netutils.h"
#include "../buffer.h"
#include "../selector.h"
#include <unistd.h>
#include <stdio.h>

void request_read_init(const unsigned state, struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    if (data != NULL) {
        request_parser_init(&data->client.request_parser);
    }
}

unsigned request_read(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    if (data == NULL) return ERROR;

    size_t n;
    uint8_t *ptr = buffer_write_ptr(&data->client_buffer, &n);
    ssize_t read = recv(key->fd, ptr, n, 0);

    if (read <= 0) {
        return ERROR;
    }

    buffer_write_adv(&data->client_buffer, read);
    request_parser_consume(&data->client.request_parser, &data->client_buffer);

    if (request_parser_is_done(&data->client.request_parser)) {
        if (request_parser_has_error(&data->client.request_parser)) {
            return ERROR;
        }

        // Por ahora solo indicamos éxito. El próximo paso será conectar con el destino.
        // usando request_process
        return DONE;
    }

    return REQUEST_READ;
}

static unsigned request_process(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    if (data == NULL) return ERROR;

    //TODO: hacer los if de cada tipo de request y qe eso llame al conect
}