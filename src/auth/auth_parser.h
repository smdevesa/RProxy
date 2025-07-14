#ifndef RPROXY_AUTH_PARSER_H
#define RPROXY_AUTH_PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "../buffer.h"

#define AUTH_VERSION 0x01
#define AUTH_MAX_USERNAME_LENGTH 255
#define AUTH_MAX_PASSWORD_LENGTH 255

enum auth_parser_state {
    AUTH_PARSER_VERSION = 0,          // Reading the version byte (must be 0x01)
    AUTH_PARSER_USERNAME_LENGTH,      // Reading the length of the username
    AUTH_PARSER_USERNAME,             // Reading the username
    AUTH_PARSER_PASSWORD_LENGTH,      // Reading the length of the password
    AUTH_PARSER_PASSWORD,             // Reading the password
    AUTH_PARSER_DONE,                 // Authentication parsing is done
    AUTH_PARSER_ERROR                 // An error occurred during parsing
};

typedef struct auth_parser {
    enum auth_parser_state state;                // Current state of the authentication parser
    char username[AUTH_MAX_USERNAME_LENGTH + 1]; // Buffer for the username
    char password[AUTH_MAX_PASSWORD_LENGTH + 1]; // Buffer for the password
    uint8_t total_bytes_to_read;                 // Total bytes expected to read in the current state
    uint8_t bytes_read;                          // Bytes read so far in the current state
    bool authenticated;                          // Flag indicating if the user is authenticated
    bool is_admin;                              // Flag indicating if the user is an admin
} auth_parser_t;

/**
 * Initializes the authentication parser.
 *
 * @param parser Pointer to the authentication parser to initialize.
 */
void auth_parser_init(auth_parser_t *parser);

/**
 * Parses the given byte array as part of the authentication process.
 *
 * @param parser Pointer to the authentication parser.
 * @param buf Pointer to the buffer containing the data to parse.
 * @return Parser state after parsing the data.
 */
enum auth_parser_state auth_parser_parse(auth_parser_t *parser, struct buffer *buf);

/**
 * Checks if the parser has completed the authentication process.
 *
 * @param parser Pointer to the authentication parser.
 * @return true if the parser has finished, false otherwise.
 */
bool auth_parser_is_done(const auth_parser_t *parser);

/**
 * Checks if an error occurred during parsing.
 *
 * @param parser Pointer to the authentication parser.
 * @return true if an error occurred, false otherwise.
 */
bool auth_parser_has_error(const auth_parser_t *parser);

/**
 * Builds the authentication response based on the parsed data.
 *
 * @param parser Pointer to the authentication parser.
 * @param buf Pointer to the buffer where the response will be written.
 * @return true if the response was built successfully, false otherwise.
 */
bool auth_parser_build_response(const auth_parser_t *parser, struct buffer *buf);

/**
 * Attempts to authenticate the user based on the parsed credentials.
 *
 * @param parser Pointer to the authentication parser.
 */
void try_to_authenticate(auth_parser_t *parser);

/**
 * Checks if the user is authenticated.
 *
 * @param parser Pointer to the authentication parser.
 * @return true if the user is authenticated, false otherwise.
 */
bool auth_parser_is_authenticated(const auth_parser_t *parser);

#endif //RPROXY_AUTH_PARSER_H
