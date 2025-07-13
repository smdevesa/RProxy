//
// Created by Tizifuchi12 on 11/7/2025.
//

#ifndef RPROXY_CLIENT_UTILS_H
#define RPROXY_CLIENT_UTILS_H

#include <stdbool.h>
#include <stdint.h>

struct auth_result {
    bool success;
    bool is_admin;
};

int connect_to_server_TCP(const char *host, const char *port);
bool handshake_socks5(int socket_fd);
bool send_auth_credentials(int socket_fd, const char *user, const char *pass);
struct auth_result recv_auth_response(int socket_fd);
bool send_connect_request(int socket_fd, const char *ip, uint16_t port);
bool recv_connect_response(int socket_fd);



#endif //RPROXY_CLIENT_UTILS_H
