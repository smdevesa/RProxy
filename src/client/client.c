// src/client/client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "client_utils.h"

#define HOST_DEFAULT "127.0.0.1"
#define PORT_DEFAULT "1080"
#define USER_DEFAULT "default_user"
#define PASS_DEFAULT "default_pass"

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <host|*> <port|*> <user|*> <pass|*>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *host     = (strcmp(argv[1], "*") == 0) ? HOST_DEFAULT : argv[1];
    const char *port     = (strcmp(argv[2], "*") == 0) ? PORT_DEFAULT : argv[2];
    const char *username = (strcmp(argv[3], "*") == 0) ? USER_DEFAULT : argv[3];
    const char *password = (strcmp(argv[4], "*") == 0) ? PASS_DEFAULT : argv[4];

    printf("Conecting %s:%s as %s...\n", host, port, username);

    int socket_fd = connect_to_server_TCP(host, port);
    if (socket_fd < 0) {
        fprintf(stderr, "Fail to connect\n");
        return EXIT_FAILURE;
    }

    printf("Connection established with server\n");

    //  handshake, auth, etc.

    close(socket_fd);
    return EXIT_SUCCESS;
}
