//
// Created by Santiago Devesa on 02/07/2025.
//

#ifndef RPROXY_SOCKS5_H
#define RPROXY_SOCKS5_H

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

#endif //RPROXY_SOCKS5_H
