#ifndef RPROXY_SOCKS5_H
#define RPROXY_SOCKS5_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>
#include "../selector.h"
#include "../handshake/handshake_parser.h"
#include "../auth/auth_parser.h"
#include "../stm.h"
#include "../buffer.h"
#include "../request/request_parser.h"

#define BUFFER_SIZE 32768

// Get the client data from the selector key
#define ATTACHMENT(key) ((struct client_data *)((key)->data))

struct client_data {
    struct state_machine sm;
    struct sockaddr_storage client_addr;
    union {
        struct handshake_parser handshake_parser;
        struct auth_parser auth_parser;
        struct request_parser request_parser;
    } client;
    bool closed;
    int client_fd;
    int origin_fd;
    struct buffer client_buffer;
    struct buffer origin_buffer;
    uint8_t client_buffer_data[BUFFER_SIZE];
    uint8_t origin_buffer_data[BUFFER_SIZE];
};

enum socks5_state {
    HANDSHAKE_READ = 0,
    HANDSHAKE_WRITE,
    AUTH_READ,
    AUTH_WRITE,
    REQUEST_READ,
    // REQUEST_DNS,
    // REQUEST_CONNECT,
    // REQUEST_WRITE,
    // COPY,
    DONE,
    ERROR
};

void socks_v5_passive_accept(struct selector_key *key);

#endif //RPROXY_SOCKS5_H
