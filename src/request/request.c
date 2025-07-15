#include "request.h"
#include "../request/request_parser.h"
#include "../socks5/socks5.h"
#include "../netutils.h"
#include "../buffer.h"
#include "../selector.h"
#include "dns_resolver.h"
#include "../users.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>

static unsigned analyze_request(struct selector_key *key);
static unsigned start_connection(struct selector_key *key);
static void handle_error(struct selector_key *key, struct client_data *data, unsigned reply_code);
static int setup_non_blocking_socket(int fd);

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
            return analyze_request(key);
        }
        request_build_response(parser, &data->origin_buffer, REQUEST_REPLY_FAILURE);
        selector_set_interest_key(key, OP_WRITE);
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

    if (request_parser_has_error(&data->client.request_parser)) {
        return ERROR;
    }
    selector_set_interest_key(key, OP_READ);
    return COPY;
}

unsigned request_connect(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    int error = 0;
    socklen_t len = sizeof(error);

    if (getsockopt(data->origin_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
        perror("getsockopt");
        error = errno;
    }

    if (error != 0) {
        close(data->origin_fd);
        data->origin_fd = -1;

        if (data->current_addrinfo && data->current_addrinfo->ai_next) {
            data->current_addrinfo = data->current_addrinfo->ai_next;
            return start_connection(key);
        }

        handle_error(key, data, REQUEST_REPLY_CONNECTION_REFUSED);
        return REQUEST_WRITE;
    }

    request_build_response(&data->client.request_parser, &data->origin_buffer, REQUEST_REPLY_SUCCESS);
    selector_set_interest_key(key, OP_WRITE);
    return REQUEST_WRITE;
}

static unsigned analyze_request(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    struct request_parser *parser = &data->client.request_parser;

    if (parser->address_type == ADDRESS_TYPE_IPV4){
        struct sockaddr_in *addr = malloc(sizeof(*addr));
        data->origin_addrinfo = calloc(1, sizeof(struct addrinfo));
        if (!addr || !data->origin_addrinfo) {
            free(addr);
            free(data->origin_addrinfo);
            perror("malloc");
            goto failure;
        }

        memcpy(&addr->sin_addr, parser->dst_addr, IPV4_LENGTH);
        addr->sin_family = AF_INET;
        addr->sin_port = htons(parser->dst_port);

        data->origin_addrinfo->ai_family = AF_INET;
        data->origin_addrinfo->ai_socktype = SOCK_STREAM;
        data->origin_addrinfo->ai_addr = (struct sockaddr*)addr;
        data->origin_addrinfo->ai_addrlen = sizeof(*addr);
        data->current_addrinfo = data->origin_addrinfo;

        return start_connection(key);
    }
    if (parser->address_type == ADDRESS_TYPE_IPV6){
        struct sockaddr_in6 *addr6 = malloc(sizeof(*addr6));
        data->origin_addrinfo = calloc(1, sizeof(struct addrinfo));
        if (!addr6 || !data->origin_addrinfo) {
            free(addr6);
            free(data->origin_addrinfo);
            perror("malloc");
            goto failure;
        }

        memcpy(&addr6->sin6_addr, parser->dst_addr, IPV6_LENGTH);
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(parser->dst_port);

        data->origin_addrinfo->ai_family = AF_INET6;
        data->origin_addrinfo->ai_socktype = SOCK_STREAM;
        data->origin_addrinfo->ai_addr = (struct sockaddr*)addr6;
        data->origin_addrinfo->ai_addrlen = sizeof(*addr6);
        data->current_addrinfo = data->origin_addrinfo;

        return start_connection(key);
    }
    if (parser->address_type == ADDRESS_TYPE_DOMAIN){
        if (parser->dst_addr_length == 0 || parser->dst_addr_length > 255) {
            goto failure_response;
        }
        dns_resolver_init(key);
        return REQUEST_DNS;
    }

failure:
    request_build_response(&data->client.request_parser, &data->origin_buffer, REQUEST_REPLY_FAILURE);
    selector_set_interest_key(key, OP_WRITE);
    return REQUEST_WRITE;

failure_response:
    request_build_response(&data->client.request_parser, &data->origin_buffer, REQUEST_REPLY_FAILURE);
    selector_set_interest_key(key, OP_WRITE);
    return REQUEST_WRITE;
}

static unsigned start_connection(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    struct addrinfo *rp;

    for(rp = data->current_addrinfo; rp != NULL; rp = rp->ai_next) {
        int fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) {
            continue;
        }
        if (setup_non_blocking_socket(fd) < 0) {
            close(fd);
            continue;
        }
        if (register_origin_selector(key, fd, data) != SELECTOR_SUCCESS) {
            close(fd);
            continue;
        }

        int ret = connect(fd, rp->ai_addr, rp->ai_addrlen);
        if (ret == 0) {
            data->origin_fd = fd;
            data->current_addrinfo = rp;
            request_build_response(&data->client.request_parser, &data->origin_buffer, REQUEST_REPLY_SUCCESS);
            selector_set_interest_key(key, OP_WRITE);
            return REQUEST_WRITE;
        }

        if (errno == EINPROGRESS) {
            data->origin_fd = fd;
            data->current_addrinfo = rp;
            selector_set_interest_key(key, OP_WRITE);
            return REQUEST_CONNECT;
        }
        close(fd);
   }

    handle_error(key, data, REQUEST_REPLY_HOST_UNREACHABLE);
    selector_set_interest_key(key, OP_WRITE);
    return REQUEST_WRITE;
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
        return -1;
    }
    return 0;
}

unsigned request_dns(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);

    if (data->dns_req.ar_result == NULL) {
        return REQUEST_DNS;
    }

    data->origin_addrinfo = data->dns_req.ar_result;
    data->current_addrinfo = data->origin_addrinfo;
    data->resolution_from_getaddrinfo = true;
    return start_connection(key);
}