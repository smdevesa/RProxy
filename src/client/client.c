// src/client/client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "client_utils.h"

#define HOST_DEFAULT "127.0.0.1"
#define PORT_DEFAULT "1080"

static void print_version() {
    printf("RProxy Client Version 1.0\n");
    printf("Developed by GROUP 12\n");
}

static void print_help(char * name) {
    printf("Usage: %s <host> <port> <username> <password> OPTION\n", name);
    printf("You can use * to use default values of host and port.\n");
    printf("Available options:\n");
    printf("  -h, --help          Show this help message and exit\n");
    printf("  -v, --version       Show version information and exit\n");
    printf("  USERS               List all users\n");
    printf("  ADD_USER <username> <password>  Add a new user with the given username and password\n");
    printf("  DELETE_USER <username>  Delete the user with the given username\n");
    printf("  CHANGE_PASSWORD <username> <new_password>  Change the password for the given user\n");
    printf("  STATS               Show server statistics\n");
    printf("  CHANGE_ROLE <username> <role>  Change the role of the user (admin/user)\n");
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return EXIT_SUCCESS;
        }

        if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
            print_version();
            return EXIT_SUCCESS;
        }
    }

    char payload[MAX_PAYLOAD_SIZE];
    size_t payload_len = build_payload_string(payload, argc, argv, 6);

    if(argc < 6) {
        print_help(argv[0]);
        return EXIT_FAILURE;
    }

    const char *host     = (strcmp(argv[1], "*") == 0) ? HOST_DEFAULT : argv[1];
    const char *port     = (strcmp(argv[2], "*") == 0) ? PORT_DEFAULT : argv[2];
    const char *username = argv[3];
    const char *password = argv[4];
    const char *option   = argv[5];

    uint8_t command_code = get_command_code(argv[5]);
    if (command_code == INVALID_COMMAND) {
        fprintf(stderr, "Invalid command: %s\n", argv[5]);
        return EXIT_FAILURE;
    }

    printf("Conecting %s:%s as %s...\n", host, port, username);

    int socket_fd = connect_to_server_TCP(host, port);
    if (socket_fd < 0) {
        fprintf(stderr, "Failed to connect\n");
        return EXIT_FAILURE;
    }
    printf("Connection established with server\n");

    if (!send_auth_credentials(socket_fd, username, password)) {
        fprintf(stderr, "Error sending credentials\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (!recv_auth_response(socket_fd)) {
        fprintf(stderr, "Autenticación fallida\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (!send_management_command(socket_fd, command_code, payload)) {
        fprintf(stderr, "Error enviando comando\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (!recv_management_response(socket_fd)) {
        fprintf(stderr, "Error recibiendo respuesta\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    close(socket_fd);
    return EXIT_SUCCESS;
}