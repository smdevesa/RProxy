//
// Created by Santiago Devesa on 02/07/2025.
//

#ifndef RPROXY_HANDSHAKE_PARSER_H
#define RPROXY_HANDSHAKE_PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include "../buffer.h"

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
 * @param buf Pointer to the buffer containing the data to parse.
 * @return Parser state after parsing the data.
 */
enum handshake_parser_state handshake_parser_parse(handshake_parser_t *parser, struct buffer *buf );

/**
 * Verifica si el parser ha completado el proceso de handshake.
 *
 * @param parser Puntero al parser de handshake.
 * @return 1 si el parser ha terminado, 0 en caso contrario.
 */
bool handshake_parser_is_done(const handshake_parser_t *parser);

/**
 * Verifica si ha ocurrido un error durante el parsing.
 *
 * @param parser Puntero al parser de handshake.
 * @return 1 si ha ocurrido un error, 0 en caso contrario.
 */
bool handshake_parser_has_error(const handshake_parser_t *parser);

/**
 * Obtiene el método de autenticación seleccionado.
 *
 * @param parser Puntero al parser de handshake.
 * @return El método de autenticación seleccionado.
 */
enum auth_methods handshake_parser_get_auth_method(const handshake_parser_t *parser);

/**
 * Construye la respuesta del handshake según el RFC1928.
 *
 * @param parser Puntero al parser de handshake.
 * @param buf Buffer donde se escribirá la respuesta.
 * @return Número de bytes escritos en el buffer.
 */
bool handshake_parser_build_response(const handshake_parser_t *parser, struct buffer *buf);

#endif //RPROXY_HANDSHAKE_PARSER_H
