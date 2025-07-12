//
// Created by Tizifuchi12 on 11/7/2025.
//

#include "client_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
}

