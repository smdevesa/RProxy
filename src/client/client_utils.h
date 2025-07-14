#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <stdbool.h>
#include <stdint.h>

#define INVALID_COMMAND 0xFF
#define MAX_PAYLOAD_SIZE 256

bool send_auth_credentials(int fd, const char *username, const char *password);
bool recv_auth_response(int fd);

bool send_management_command(int fd, uint8_t cmd, const char *args);
bool recv_management_response(int fd, char *output, size_t max_len);

int connect_to_server_TCP(const char *host, const char *port);

uint8_t get_command_code(const char *option);
size_t build_payload_string(char *dest, int argc, char *argv[], int start_index);

#endif
