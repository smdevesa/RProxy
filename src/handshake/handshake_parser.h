//
// Created by Santiago Devesa on 02/07/2025.
//

#ifndef RPROXY_HANDSHAKE_PARSER_H
#define RPROXY_HANDSHAKE_PARSER_H

#include <stddef.h>

enum auth_methods {
    NO_AUTH = 0x00,
    USER_PASS = 0x02,
    NO_ACCEPTABLE = 0xFF
};

enum handshake_parser_state {
    HANDSHAKE_PARSER_VERSION = 0, // Reading the version byte (must be 0x05)
    HANDSHAKE_PARSER_NMETHODS, // Reading the number of authentication methods
    HANDSHAKE_PARSER_METHODS, // Reading the authentication methods
    HANDSHAKE_PARSER_DONE, // Handshake parsing is done
    HANDSHAKE_PARSER_ERROR // An error occurred during parsing
};

typedef struct handshake_parser {
    enum handshake_parser_state state; // Current state of the handshake parser
    enum auth_methods selected_method; // The selected authentication method
    size_t methods_count; // Number of authentication methods received
} handshake_parser_t;

/**
 * Initializes the handshake parser.
 *
 * @param parser Pointer to the handshake parser to initialize.
 */
void handshake_parser_init(handshake_parser_t *parser);

/**
 * Parses the given byte array as part of the handshake process.
 *
 * @param parser Pointer to the handshake parser.
 * @param data Pointer to the byte array containing the handshake data.
 * @return Parser state after parsing the data.
 */
enum handshake_parser_state handshake_parser_parse(handshake_parser_t *parser, const unsigned char *data, size_t length);



#endif //RPROXY_HANDSHAKE_PARSER_H
