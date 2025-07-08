#include "request.h"

#include "../request/request_parser.h"
#include "../socks5/socks5.h"
#include "../netutils.h"
#include "../buffer.h"
#include "../selector.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static unsigned analyze_request(struct selector_key *key);
static unsigned start_connection(struct selector_key *key);

void request_read_init(const unsigned state, struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    if (data != NULL) {
        request_parser_init(&data->client.request_parser);
    }
}

unsigned request_read(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    struct request_parser *parser = &data->client.request_parser;

    size_t read_limit;
    ssize_t read_count;
    uint8_t *read_buffer = buffer_write_ptr(&data->client_buffer, &read_limit);
    read_count = recv(key->fd, read_buffer, read_limit, 0);

    if (read_count <= 0) {
        return ERROR;
    }

    buffer_write_adv(&data->client_buffer, read_count);
    request_parser_consume(parser, &data->client_buffer);

    if (request_parser_is_done(parser)) {
        if (!request_parser_has_error(parser)){
            analyze_request(key);
        } else {
            request_build_response(parser, &data->origin_buffer, REQUEST_REPLY_FAILURE);
            if (selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS) {
                perror("selector_set_interest");
                return ERROR;
            }
            return REQUEST_WRITE;
        }
        if (selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS || !request_build_response(parser, &data->origin_buffer, REQUEST_REPLY_SUCCESS)) {
            perror("selector_set_interest");
            return ERROR;
        }
        return REQUEST_WRITE;
    }
    return REQUEST_READ;
}

unsigned request_write(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    size_t write_limit;
    ssize_t write_count;
    uint8_t *write_buffer = buffer_read_ptr(&data->origin_buffer, &write_limit);

    write_count = send(key->fd, write_buffer, write_limit, MSG_NOSIGNAL);
    if (write_count <= 0) {
        perror("send");
        return ERROR;
    }

    buffer_read_adv(&data->origin_buffer, write_count);
    if (buffer_can_read(&data->origin_buffer)) {
        return REQUEST_WRITE;
    }

    if (request_parser_has_error(&data->client.request_parser) || selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) {
        return ERROR;
    }

    return COPY;
}

unsigned request_connect(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    int error = 0;
    socklen_t len = sizeof(error);

    if (getsockopt(data->origin_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
        request_build_response(&data->client.request_parser, &data->origin_buffer, REQUEST_REPLY_CONNECTION_REFUSED);
        if (selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS) {
            return ERROR;
        }
        return REQUEST_WRITE;
    }

    return start_connection(key);
}

static unsigned analyze_request(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    struct request_parser *parser = &data->client.request_parser;

    if (parser->address_type == ADDRESS_TYPE_IPV4){
        struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));
        data->origin_addrinfo = calloc(1, sizeof(struct addrinfo));
        if (addr == NULL || data->origin_addrinfo == NULL) {
            if (addr != NULL) free(addr);
            if (data->origin_addrinfo != NULL) free(data->origin_addrinfo);
            perror("malloc or calloc");
            goto finally;
        }

        memcpy(&addr->sin_addr, parser->dst_addr, IPV4_LENGTH);
        addr->sin_family = AF_INET;
        addr->sin_port = htons(parser->dst_port);

        data->origin_addrinfo->ai_family = AF_INET;
        data->origin_addrinfo->ai_socktype = SOCK_STREAM;
        data->origin_addrinfo->ai_addr = (struct sockaddr *)addr;
        data->origin_addrinfo->ai_addrlen = sizeof(struct sockaddr_in);

        return start_connection(key);
    }
    if (parser->address_type == ADDRESS_TYPE_IPV6){
        struct sockaddr_in6* addr6 = malloc(sizeof(struct sockaddr_in6));
        data->origin_addrinfo = calloc(1, sizeof(struct addrinfo));
        if (addr6 == NULL || data->origin_addrinfo == NULL){
            if (addr6 != NULL) free(addr6);
            if (data->origin_addrinfo != NULL) free(data->origin_addrinfo);
            perror("malloc or calloc");
            goto finally;
        }

        memcpy(&addr6->sin6_addr, parser->dst_addr, IPV6_LENGTH);
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(parser->dst_port);

        data->origin_addrinfo->ai_family = AF_INET6;
        data->origin_addrinfo->ai_socktype = SOCK_STREAM;
        data->origin_addrinfo->ai_addr = (struct sockaddr*)addr6;
        data->origin_addrinfo->ai_addrlen = sizeof(struct sockaddr_in6);

        return start_connection(key);
    }
    if (parser->address_type == ADDRESS_TYPE_DOMAIN){
        // Domain name handling. Let's simulate a DNS resolution
        return REQUEST_DNS;
    }
    // If we reach here, the address type is not supported
    request_build_response(parser, &data->origin_buffer, REQUEST_REPLY_ADDRESS_TYPE_NOT_SUPPORTED);
    if (selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS) {
        perror("selector_set_interest");
        return ERROR;
    }
    return REQUEST_WRITE;

    finally:
    request_build_response(&data->client.request_parser, &data->origin_buffer, REQUEST_REPLY_FAILURE);
}


static void handle_error(struct selector_key *key, struct client_data *data, unsigned reply_code) {
    request_build_response(&data->client.request_parser, &data->origin_buffer, reply_code);
    if (selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS) {
        perror("selector_set_interest");
    }
}

static int setup_non_blocking_socket(int fd) {
    if (selector_fd_set_nio(fd) == -1) {
        perror("selector_fd_set_nio");
        close(fd);
        return -1;
    }
    return 0;
}

static unsigned start_connection(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);

    data->origin_fd = socket(data->origin_addrinfo->ai_family, data->origin_addrinfo->ai_socktype, data->origin_addrinfo->ai_protocol);
    if (data->origin_fd < 0) {
        perror("socket");
        handle_error(key, data, REQUEST_REPLY_FAILURE);
        return REQUEST_WRITE;
    }

    if (setup_non_blocking_socket(data->origin_fd) < 0) {
        handle_error(key, data, REQUEST_REPLY_FAILURE);
        return REQUEST_WRITE;
    }

    if (register_origin_selector(key, data->origin_fd, data) != SELECTOR_SUCCESS) {
        close(data->origin_fd);
        data->origin_fd = -1; // Reset origin_fd on failure
        perror("register_origin_selector");
        handle_error(key, data, REQUEST_REPLY_FAILURE);
        return REQUEST_WRITE;
    }

    if (connect(data->origin_fd, data->origin_addrinfo->ai_addr, data->origin_addrinfo->ai_addrlen) < 0) {
        if (errno == EINPROGRESS) {
            if (selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS) {
                return ERROR;
            }
            return REQUEST_CONNECT; // Connection in progress
        } else {
            handle_error(key, data, REQUEST_REPLY_CONNECTION_REFUSED);
            return REQUEST_WRITE;
        }
    }

    // Connection successful
    request_build_response(&data->client.request_parser, &data->origin_buffer, REQUEST_REPLY_SUCCESS);
    if (selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS) {
        return ERROR;
    }
    return REQUEST_WRITE;
}


// Por ahora, el DNS ya esta resuelto para probar - TODO hacerlo bien
unsigned request_DNS_completed(struct selector_key *key) {
    return REQUEST_CONNECT;
}