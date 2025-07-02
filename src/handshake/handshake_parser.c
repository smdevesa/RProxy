#include "handshake_parser.h"
#include "../parser.h"

#define SOCKS5_VERSION 0x05

typedef enum handshake_parser_state (*state_handler)(handshake_parser_t *parser, uint8_t c);

// State handlers
static enum handshake_parser_state parse_version(handshake_parser_t *parser, uint8_t c);
static enum handshake_parser_state parse_nmethod(handshake_parser_t *parser, uint8_t c);
static enum handshake_parser_state parse_methods(handshake_parser_t *parser, uint8_t c);
static enum handshake_parser_state parse_done(handshake_parser_t *parser, uint8_t c);
static enum handshake_parser_state parse_error(handshake_parser_t *parser, uint8_t c);

static state_handler state_handlers[] = {
    (state_handler) parse_version,  // HANDSHAKE_PARSER_VERSION
    (state_handler) parse_nmethod,  // HANDSHAKE_PARSER_NMETHODS
    (state_handler) parse_methods,  // HANDSHAKE_PARSER_METHODS
    (state_handler) parse_done,     // HANDSHAKE_PARSER_DONE
    (state_handler) parse_error     // HANDSHAKE_PARSER_ERROR
};

void handshake_parser_init(handshake_parser_t *parser) {
    if (parser == NULL) {
        return;
    }
    parser->state = HANDSHAKE_PARSER_VERSION;
    parser->selected_method = NO_ACCEPTABLE;
    parser->methods_count = 0;
}

enum handshake_parser_state handshake_parser_parse(handshake_parser_t *parser, struct buffer *buf ) {
    while (buffer_can_read(buf) && !handshake_parser_is_done(parser)) {
        parser->state = state_handlers[parser->state](parser, buffer_read(buf));
    }
    return parser->state;
}

bool handshake_parser_is_done(const handshake_parser_t *parser) {
    return parser != NULL && (parser->state == HANDSHAKE_PARSER_DONE || parser->state == HANDSHAKE_PARSER_ERROR);
}

bool handshake_parser_has_error(const handshake_parser_t *parser) {
    return parser != NULL && parser->state == HANDSHAKE_PARSER_ERROR;
}

enum auth_methods handshake_parser_get_auth_method(const handshake_parser_t *parser) {
    return parser != NULL ? parser->selected_method : NO_ACCEPTABLE;
}

bool handshake_parser_build_response(const handshake_parser_t *parser, struct buffer *buf) {
    if (!buffer_can_write(buf) || parser == NULL || parser->state != HANDSHAKE_PARSER_DONE) {
        return false;
    }
    buffer_write(buf, SOCKS5_VERSION); // Version
    if (!buffer_can_write(buf)) {
        return false;
    }
    buffer_write(buf, parser->selected_method); // Selected method
    return true;
}

static enum handshake_parser_state parse_version(handshake_parser_t *parser, uint8_t c) {
    if (c == SOCKS5_VERSION) {
        return HANDSHAKE_PARSER_NMETHODS;
    }
    return HANDSHAKE_PARSER_ERROR;
}

static enum handshake_parser_state parse_nmethod(handshake_parser_t *parser, uint8_t c) {
    if (c > 0) {
        parser->methods_count = c;
        return HANDSHAKE_PARSER_METHODS;
    }
    return HANDSHAKE_PARSER_ERROR;
}

static enum handshake_parser_state parse_methods(handshake_parser_t *parser, uint8_t c) {
    if (c == USER_PASS) {
        parser->selected_method = USER_PASS;
    }
    else if (c == NO_AUTH && parser->selected_method == NO_ACCEPTABLE) {
        parser->selected_method = NO_AUTH;
    }
    parser->methods_count--;
    if (parser->methods_count == 0 && parser->selected_method == NO_ACCEPTABLE) {
        return HANDSHAKE_PARSER_ERROR; // No acceptable methods found
    }
    return parser->methods_count > 0 ? HANDSHAKE_PARSER_METHODS : HANDSHAKE_PARSER_DONE;
}

static enum handshake_parser_state parse_done(handshake_parser_t *parser, uint8_t c) {
    // This state should not receive any more data
    c += 0;
    return HANDSHAKE_PARSER_DONE;
}

static enum handshake_parser_state parse_error(handshake_parser_t *parser, uint8_t c) {
    // This state should not receive any more data
    c += 0;
    return HANDSHAKE_PARSER_ERROR;
}









