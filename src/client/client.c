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


    if (!handshake_socks5(socket_fd)) {
        fprintf(stderr, "Error en handshake SOCKS5\n");
        close(socket_fd);
        return 1;
    }
    printf("Handshake SOCKS5 successful\n");

    if (!send_auth_credentials(socket_fd, username, password)) {
        fprintf(stderr, "Error enviando credenciales.\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (!recv_auth_response(socket_fd)) {
        fprintf(stderr, "Autenticación fallida.\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    printf("Autenticación exitosa.\n");

    //esto era para probar el connect, hacer el bucle aca


    if (!send_connect_request(socket_fd, "142.251.128.46", 80)) {
        fprintf(stderr, "Fallo el envio del connect\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (!recv_connect_response(socket_fd)) {
        fprintf(stderr, "El servidor rechazó la conexión\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }
    printf("Túnel conectado al destino exitosamente\n");



    close(socket_fd);
    return EXIT_SUCCESS;
}
