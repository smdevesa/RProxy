#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    const char *name;
    uint8_t code;
    uint8_t argc_expected;
    uint8_t needs_admin;
} command_info_t;

static const command_info_t commands[] = {
        { "USERS",           0x00, 0, 0 },
        { "ADD_USER",        0x01, 2, 1 },
        { "DELETE_USER",     0x02, 1, 1 },
        { "CHANGE_PASSWORD", 0x03, 2, 1 },
        { "STATS",           0x04, 0, 0 },
        { "CHANGE_ROLE",     0x05, 2, 1 },
        {"SET_DEFAULT_AUTH_METHOD", 0x06, 1, 1 },
        {"GET_DEFAULT_AUTH_METHOD", 0x07, 1, 1 },
};

#define COMMANDS_COUNT (sizeof(commands) / sizeof(commands[0]))

#define INVALID_COMMAND 0xFF
#define MAX_PAYLOAD_SIZE 256

bool send_auth_credentials(int fd, const char *username, const char *password);
bool recv_auth_response(int fd);

bool send_management_command(int fd, uint8_t cmd, const char *args);
bool recv_management_response(int fd, char *output, size_t max_len);

int connect_to_server_TCP(const char *host, const char *port);

const command_info_t *get_command_info(const char *name);
size_t build_payload_string(char *dest, int argc, char *argv[], int start_index);

#endif
