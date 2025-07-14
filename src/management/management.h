#ifndef MANAGEMENT_H
#define MANAGEMENT_H

/*
 *
 *          PROTOCOL MANAGEMENT
 *
 *              REQUEST
 *          CLIENT->SERVER
 *  ---------------------------------------------
 *  | VERSION | COMMAND | PAYLOAD-LEN | PAYLOAD |
 *  ---------------------------------------------
 *  |    1    |    1    |      1      |  0-255  |
 *     0x01
 *
 *
 *                RESPONSE
 *             SERVER->CLIENT
 *          --------------------
 *          | VERSION | STATUS |
 *          --------------------
 *          |    1    |    1   |
 *
 *
 */

#include "management_command_parser.h"

#define ATTACHMENT(key) ((struct management_client *)((key)->data))

#define REQUEST_BUFFER_SIZE 512
#define RESPONSE_BUFFER_SIZE 8196

typedef enum {
    MANAGEMENT_AUTH = 0,
    MANAGEMENT_AUTH_READ,
    MANAGEMENT_AUTH_WRITE,
    MANAGEMENT_REQUEST_READ,
    MANAGEMENT_REQUEST_WRITE,
    MANAGEMENT_CLOSED,
    MANAGEMENT_ERROR,
} management_state;

typedef struct management_client {
    struct state_machine *sm;

    union {
        struct  auth_parser auth_parser; // For authentication
        management_command_parser request_parser; // For management commands
    } management_parser;

    bool closed; // Indicates if the connection is closed
    int client_fd; // File descriptor for the client connection

    struct buffer request_buffer; // Buffer for incoming requests
    struct buffer response_buffer; // Buffer for outgoing responses
    uint8_t request_buffer_data[REQUEST_BUFFER_SIZE]; // Data for request buffer
    uint8_t response_buffer_data[RESPONSE_BUFFER_SIZE]; // Data for response buffer

    management_command command; // Current management command being processed
    bool is_admin; // Indicates if the user is an admin

} management_client;

void management_passive_accept(struct selector_key *key);

#endif //MANAGEMENT_H
