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
    const char * args_expected;
} command_info_t;

static const command_info_t commands[] = {
        { "USERS",           0x00, 0, 0, NULL },
        { "ADD_USER",        0x01, 2, 1 , "<username> <password>" },
        { "DELETE_USER",     0x02, 1, 1 , "<username>" },
        { "CHANGE_PASSWORD", 0x03, 2, 1 , "<username> <new_password>" },
        { "STATS",           0x04, 0, 0 , NULL },
        { "CHANGE_ROLE",     0x05, 2, 1 , "<username> <admin|user>" },
        {"SET_DEFAULT_AUTH_METHOD", 0x06, 1, 1 , "<no_auth|username_password>" },
        {"GET_DEFAULT_AUTH_METHOD", 0x07, 0, 1 , NULL },
        { "VIEW_ACTIVITY_LOG", 0x08, 1, 1, "<username>" },
};

#define COMMANDS_COUNT (sizeof(commands) / sizeof(commands[0]))

#define INVALID_COMMAND 0xFF
#define MAX_PAYLOAD_SIZE 256


/**
 * Sends authentication credentials to the server.
 * @param fd The file descriptor of the connection.
 * @param username The username to authenticate.
 * @param  password The password to authenticate.
 */
bool send_auth_credentials(int fd, const char *username, const char *password);

/**
 * Receives the authentication response from the server.
 * @param fd The file descriptor of the connection.
 * @return true if authentication was successful, false otherwise.
 */
bool recv_auth_response(int fd);


/**
 * Sends a management command to the server.
 * @param fd The file descriptor of the connection.
 * @param cmd The command code to send.
 * @param args The arguments for the command, if any.
 * @return true if the command was sent successfully, false otherwise.
 */
bool send_management_command(int fd, uint8_t cmd, const char *args);

/**
 * Receives a management response from the server.
 * @param fd The file descriptor of the connection.
 * @param output Buffer to store the response.
 * @param max_len Maximum length of the output buffer.
 * @return true if the response was received successfully, false otherwise.
 */
bool recv_management_response(int fd, char *output, size_t max_len);


/**
 * Connects to a server using TCP.
 * @param host The hostname or IP address of the server.
 * @param port The port number to connect to.
 * @return The file descriptor of the connected socket, or -1 on error.
 */
int connect_to_server_TCP(const char *host, const char *port);

/** Gets the command information by name.
 * @param name The name of the command.
 * @return Pointer to the command_info_t structure if found, NULL otherwise.
 */
const command_info_t *get_command_info(const char *name);

/**
 * Builds a payload string from command arguments.
 * @param dest The destination buffer to store the payload.
 * @param argc The number of arguments.
 * @param argv The array of argument strings.
 * @param start_index The index to start reading arguments from.
 * @return The length of the built payload string.
 */
size_t build_payload_string(char *dest, int argc, char *argv[], int start_index);

#endif
