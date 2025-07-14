#include "client_utils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>

static ssize_t send_all(int fd, const void *buf, size_t len) {
    size_t total_sent = 0;
    const uint8_t *ptr = buf;

    while (total_sent < len) {
        ssize_t sent = send(fd, ptr + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            perror("send");
            return -1;
        }
        total_sent += sent;
    }
    return total_sent;
}

ssize_t recv_all(int fd, void *buf, size_t len) {
    size_t total_read = 0;
    uint8_t *ptr = buf;

    while (total_read < len) {
        ssize_t r = recv(fd, ptr + total_read, len - total_read, 0);
        if (r <= 0) {
            perror("recv");
            return -1;
        }
        total_read += r;
    }
    return total_read;
}

int connect_to_server_TCP(const char *host, const char *port) {
    struct addrinfo addr;
    struct addrinfo *res = NULL;
    memset(&addr, 0, sizeof(addr));

    addr.ai_family = AF_UNSPEC;
    addr.ai_socktype = SOCK_STREAM;
    addr.ai_protocol = IPPROTO_TCP;

    int err = getaddrinfo(host, port, &addr, &res);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(err));
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

bool send_auth_credentials(int fd, const char *username, const char *password) {
    uint8_t version = 0x01;
    uint8_t user_len = strlen(username);
    uint8_t pass_len = strlen(password);

    if (user_len > 255 || pass_len > 255) {
        fprintf(stderr, "Username or password too long\n");
        return false;
    }

    uint8_t buffer[2 + 255 + 1 + 255];

    size_t offset = 0;
    buffer[offset++] = version;
    buffer[offset++] = user_len;
    memcpy(buffer + offset, username, user_len);
    offset += user_len;
    buffer[offset++] = pass_len;
    memcpy(buffer + offset, password, pass_len);
    offset += pass_len;

    return send_all(fd, buffer, offset) == (ssize_t)offset;
}

bool recv_auth_response(int fd) {
    uint8_t resp[2];
    ssize_t r = recv_all(fd, resp, 2);
    return r == 2 && resp[0] == 0x01 && resp[1] == 0x00;
}

bool send_management_command(int fd, uint8_t cmd, const char *args) {
    uint8_t version = 0x01;
    uint8_t len = args ? strlen(args) : 0;
    if (len > 255) {
        fprintf(stderr, "Payload too long\n");
        return false;
    }

    uint8_t buffer[3 + 255];
    buffer[0] = version;
    buffer[1] = cmd;
    buffer[2] = len;
    if (args && len > 0) {
        memcpy(buffer + 3, args, len);
    }

    return send_all(fd, buffer, 3 + len) == (ssize_t)(3 + len);
}

bool recv_management_response(int fd, char *output, size_t max_len) {
    uint8_t header[2];
    ssize_t r = recv_all(fd, header, 2);
    if (r != 2 || header[0] != 0x01) {
        return false;
    }

    size_t received = 0;
    while (received < max_len - 1) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n <= 0 || c == '\0') break;
        output[received++] = c;
    }
    output[received] = '\0';
    return true;
}

uint8_t get_command_code(const char *option) {
    if (strcmp(option, "USERS") == 0) return 0x00;
    if (strcmp(option, "ADD_USER") == 0) return 0x01;
    if (strcmp(option, "DELETE_USER") == 0) return 0x02;
    if (strcmp(option, "CHANGE_PASSWORD") == 0) return 0x03;
    if (strcmp(option, "STATS") == 0) return 0x04;
    if (strcmp(option, "CHANGE_ROLE") == 0) return 0x05;

    return INVALID_COMMAND;
}

size_t build_payload_string(char *dest, int argc, char *argv[], int start_index) {
    size_t total = 0;
    size_t max_len = MAX_PAYLOAD_SIZE;

    for (int i = start_index; i < argc; i++) {
        size_t arg_len = strlen(argv[i]);
        if (total + arg_len + 1 >= max_len) {  // +1 for ':' or '\0'
            fprintf(stderr, "Payload too long\n");
            return 0;
        }

        memcpy(dest + total, argv[i], arg_len);
        total += arg_len;

        if (i < argc - 1) {
            dest[total++] = ':';
        }
    }

    dest[total] = '\0';
    return total;
}
