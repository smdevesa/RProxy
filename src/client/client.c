// src/client/client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "client_utils.h"

#define HOST_DEFAULT "127.0.0.1"
#define PORT_DEFAULT "8080"

static void print_version() {
    printf("RProxy Client Version 1.0\n");
    printf("Developed by GROUP 12\n");
}

static void print_command_info() {
    printf("Available commands:\n");
    for (size_t i = 0; i < COMMANDS_COUNT; ++i) {
        printf("  %-25s %d args %s\n",
               commands[i].name,
               commands[i].argc_expected,
               commands[i].needs_admin ? "(admin only)" : "");
    }
}

static void print_help(char * name) {
    printf("Usage: %s <host> <port> <username> <password> COMMAND [args]\n", name);
    printf("You can use $ to use default values of host and port.\n");
    print_command_info();
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

    if (argc < 6) {
        print_help(argv[0]);
        return EXIT_FAILURE;
    }

    const command_info_t *info = get_command_info(argv[5]);
    if (info == NULL) {
        fprintf(stderr, "Invalid command: %s\n", argv[5]);
        return EXIT_FAILURE;
    }

    int args_provided = argc - 6;
    if (args_provided < info->argc_expected) {
        fprintf(stderr, "Command '%s' expects %d arguments, got %d\n",
                info->name, info->argc_expected, args_provided);
        return EXIT_FAILURE;
    }

    char payload[MAX_PAYLOAD_SIZE];
    size_t payload_len = build_payload_string(payload, argc, argv, 6);
    if (payload_len == 0 && info->argc_expected > 0) {
        fprintf(stderr, "Failed to build command payload\n");
        return EXIT_FAILURE;
    }

    uint8_t command_code = info->code;


    const char *host     = (strcmp(argv[1], "$") == 0) ? HOST_DEFAULT : argv[1];
    const char *port     = (strcmp(argv[2], "$") == 0) ? PORT_DEFAULT : argv[2];
    const char *username = argv[3];
    const char *password = argv[4];

    printf("Connecting to %s:%s as %s...\n", host, port, username);

    int socket_fd = connect_to_server_TCP(host, port);
    if (socket_fd < 0) {
        fprintf(stderr, "Failed to connect\n");
        return EXIT_FAILURE;
    }
    printf("Connection established with server\n");

    if (!send_auth_credentials(socket_fd, username, password)) {
        fprintf(stderr, "Error sending authentication credentials\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (!recv_auth_response(socket_fd)) {
        fprintf(stderr, "Auth failed\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (!send_management_command(socket_fd, command_code, payload)) {
        fprintf(stderr, "Error sending management command\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    char buffer[1024];

    if (!recv_management_response(socket_fd,buffer, sizeof(buffer))) {
        fprintf(stderr, "Error receiving server response\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    printf("%s\n", buffer);

    close(socket_fd);
    return EXIT_SUCCESS;
}