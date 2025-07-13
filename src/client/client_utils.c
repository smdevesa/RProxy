#include "client_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

int connect_to_server_TCP(const char *host, const char *port) {
    struct addrinfo addr;
    struct addrinfo *res = NULL;
    memset(&addr, 0, sizeof(addr));

    addr.ai_family = AF_UNSPEC;
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
            continue;
        }

        if (connect(socket_fd, p->ai_addr, p->ai_addrlen) != 0) {
            close(socket_fd);
            socket_fd = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(res);
    return socket_fd;
}

bool handshake_socks5(int socket_fd) {
    uint8_t buffer[3] = {0x05, 0x01, 0x02};
    if (write(socket_fd, buffer, 3) != 3) return false;

    uint8_t response[2];
    if (read(socket_fd, response, 2) != 2) return false;

    return response[0] == 0x05 && response[1] == 0x02;
}

bool send_auth_credentials(int socket_fd, const char *user, const char *pass) {
    uint8_t buffer[513]; //DESPUES CAMBIR E MAGIC NUMBER
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


struct auth_result recv_auth_response(int socket_fd) {
    struct auth_result result = { .success = false, .is_admin = false };

    uint8_t version, status;
    if (read(socket_fd, &version, 1) != 1) return result;
    if (read(socket_fd, &status, 1) != 1) return result;

    result.success = (status & 0x7F) == 0x00;
    result.is_admin = (status & 0x80) != 0;

    if (result.success) {
        printf("Autenticación exitosa. Rol: %s\n", result.is_admin ? "admin" : "usuario");
    } else {
        printf("Autenticación fallida.\n");
    }

    return result;
}


bool send_connect_request(int socket_fd, const char *ip, uint16_t port) {
    uint8_t buffer[10];
    buffer[0] = 0x05; // SOCKS5
    buffer[1] = 0x01; // CONNECT
    buffer[2] = 0x00; // Reserved
    buffer[3] = 0x01; // IPv4

    if (inet_pton(AF_INET, ip, &buffer[4]) != 1) return false;

    buffer[8] = (uint8_t)(port >> 8);
    buffer[9] = (uint8_t)(port & 0xFF);

    return write(socket_fd, buffer, 10) == 10;
}

bool recv_connect_response(int socket_fd) {
    uint8_t buffer[10];
    if (read(socket_fd, buffer, 10) != 10) return false;
    return buffer[1] == 0x00; // 0x00 = succeeded
}
