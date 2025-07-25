#ifndef RPROXY_SOCKS5_H
#define RPROXY_SOCKS5_H

#include <stdbool.h>
#include <sys/socket.h>
#include <netdb.h>
#include "../selector.h"
#include "../handshake/handshake_parser.h"
#include "../auth/auth_parser.h"
#include "../stm.h"
#include "../buffer.h"
#include "../request/request_parser.h"

#define BUFFER_SIZE 32768

// Get the client data from the selector key
#define ATTACHMENT(key) ((struct client_data *)((key)->data))

struct  client_data {
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
    struct addrinfo * origin_addrinfo; // Used for DNS resolution
    struct addrinfo * current_addrinfo; // Current address being tried
    uint8_t client_buffer_data[BUFFER_SIZE];
    uint8_t origin_buffer_data[BUFFER_SIZE];
    fd_selector selector;
    struct gaicb dns_req;
    char dns_host[256];
    char dns_port[6];
    char username[65]; // +1 for null terminated
    bool resolution_from_getaddrinfo;
    bool is_admin; //TODO Usage?
    bool access_registered; // Indicates if the user access has been registered
};

enum socks5_state {
    HANDSHAKE_READ = 0,
    HANDSHAKE_WRITE,
    AUTH_READ,
    AUTH_WRITE,
    REQUEST_READ,
    REQUEST_DNS,
    REQUEST_CONNECT,
    REQUEST_WRITE,
    COPY,
    DONE,
    ERROR
};

/**
 * closes the connection and cleans up resources
 * @param key the selector key containing the client data
 */
void close_connection(struct selector_key *key);

/**
 * Handles the passive accept for SOCKS5 connections.
 * @param key The selector key containing the client data.
 */
void socks_v5_passive_accept(struct selector_key *key);

/**
 * aux function to handle the connection process
 * @param key The selector key containing the client data.
 * @param origin_fd The file descriptor of the origin server.
 * @param data The client data structure containing the connection information.
 */
selector_status register_origin_selector(struct selector_key *key, int origin_fd, struct client_data *data);

#endif //RPROXY_SOCKS5_H
