//
// Created by Tizifuchi12 on 11/7/2025.
//

#include "client_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

int connect_to_server_TCP(const char *host, const char *port) {
    struct addrinfo addr;
    struct addrinfo *res = NULL;
    memset(&addr, 0, sizeof(addr));

    addr.ai_family = AF_UNSPEC; // Allow IPv4 or IPv6
    addr.ai_socktype = SOCK_STREAM;
    addr.ai_protocol = IPPROTO_TCP;

    int err = getaddrinfo(host, port, &addr, &res);
    if (err != 0) {
        return -1;
    }

    int socket_fd = -1;
    for(struct addrinfo *p = res; p != NULL && socket_fd == -1; p = p->ai_next) {
        socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socket_fd < 0) {
            continue; //no encontro
        }

        if (connect(socket_fd, p->ai_addr, p->ai_addrlen) != 0) {
            close(socket_fd);
            socket_fd = -1;
            continue;
        }
    }

        freeaddrinfo(res); // Free the linked list of addresses
        return socket_fd; // Successfully connected
}


bool handshake_socks5(int socket_fd) {
    uint8_t request[3] = {0x05, 0x01, 0x02};  // VERSION, NMETHODS, METHODS
    ssize_t sent = send(socket_fd, request, sizeof(request), 0);
    if (sent != sizeof(request)) {
        return false;
    }

    uint8_t response[2];
    ssize_t received = recv(socket_fd, response, sizeof(response), 0);
    if (received != 2) {
        return false;
    }

    return response[0] == 0x05 && response[1] == 0x02; //es que respondio ok
}

bool send_auth_credentials(int socket_fd, const char *user, const char *pass) {
    uint8_t buffer[513]; // 1 + 1 + 255 + 1 + 255
    size_t offset = 0;

    size_t user_len = strlen(user);
    size_t pass_len = strlen(pass);
    if (user_len > 255 || pass_len > 255) return false;

    buffer[offset++] = 0x01;           // Version
    buffer[offset++] = (uint8_t)user_len;
    memcpy(&buffer[offset], user, user_len);
    offset += user_len;
    buffer[offset++] = (uint8_t)pass_len;
    memcpy(&buffer[offset], pass, pass_len);
    offset += pass_len;

    return write(socket_fd, buffer, offset) == (ssize_t)offset;
}

bool recv_auth_response(int socket_fd) {
    uint8_t response[2];
    ssize_t n = read(socket_fd, response, 2);
    return (n == 2 && response[0] == 0x01 && response[1] == 0x00);
}

bool send_connect_request(int socket_fd, const char *ip, uint16_t port) {
    uint8_t request[10];
    request[0] = 0x05;  // VER
    request[1] = 0x01;  // CMD = CONNECT
    request[2] = 0x00;  // RSV
    request[3] = 0x01;  // ATYP = IPv4

    if (inet_pton(AF_INET, ip, &request[4]) != 1) {
        return false;
    }

    request[8] = (port >> 8) & 0xFF;
    request[9] = port & 0xFF;

    return write(socket_fd, request, sizeof(request)) == sizeof(request);
}


bool recv_connect_response(int socket_fd) {
    uint8_t response[10];
    ssize_t n = read(socket_fd, response, sizeof(response));
    return (n == 10 && response[1] == 0x00);  // REP = 0x00 es exito
}

