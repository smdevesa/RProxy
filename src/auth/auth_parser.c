#include "auth_parser.h"
#include "../users.h"

typedef enum auth_parser_state (*state_handler)(auth_parser_t *parser, uint8_t c);

// State handlers
static enum auth_parser_state parse_version(auth_parser_t *parser, uint8_t c);
static enum auth_parser_state parse_username_length(auth_parser_t *parser, uint8_t c);
static enum auth_parser_state parse_username(auth_parser_t *parser, uint8_t c);
static enum auth_parser_state parse_password_length(auth_parser_t *parser, uint8_t c);
static enum auth_parser_state parse_password(auth_parser_t *parser, uint8_t c);
static enum auth_parser_state parse_done(auth_parser_t *parser, uint8_t c);
static enum auth_parser_state parse_error(auth_parser_t *parser, uint8_t c);

static state_handler state_handlers[] = {
    (state_handler) parse_version,          // AUTH_PARSER_VERSION
    (state_handler) parse_username_length,  // AUTH_PARSER_USERNAME_LENGTH
    (state_handler) parse_username,         // AUTH_PARSER_USERNAME
    (state_handler) parse_password_length,  // AUTH_PARSER_PASSWORD_LENGTH
    (state_handler) parse_password,         // AUTH_PARSER_PASSWORD
    (state_handler) parse_done,             // AUTH_PARSER_DONE
    (state_handler) parse_error             // AUTH_PARSER_ERROR
};

void auth_parser_init(auth_parser_t *parser) {
    if (parser == NULL) {
        return;
    }
    parser->state = AUTH_PARSER_VERSION;
    parser->authenticated = false;
    parser->username[0] = '\0';
    parser->password[0] = '\0';
    parser->bytes_read = 0;
}

enum auth_parser_state auth_parser_parse(auth_parser_t *parser, struct buffer *buf) {
    while (buffer_can_read(buf) && !auth_parser_is_done(parser)) {
        parser->state = state_handlers[parser->state](parser, buffer_read(buf));
    }
    return parser->state;
}

bool auth_parser_is_done(const auth_parser_t *parser) {
    return parser != NULL && (parser->state == AUTH_PARSER_DONE || parser->state == AUTH_PARSER_ERROR);
}

bool auth_parser_has_error(const auth_parser_t *parser) {
    return parser != NULL && parser->state == AUTH_PARSER_ERROR;
}

bool auth_parser_build_response(const auth_parser_t *parser, struct buffer *buf) {
    if (!buffer_can_write(buf) || parser == NULL || parser->state != AUTH_PARSER_DONE) {
        return false;
    }

    if (parser->authenticated) {
        buffer_write(buf, AUTH_VERSION); // Version
        if (!buffer_can_write(buf)) {
            return false;
        }
        buffer_write(buf, 0x00); // Success
    } else {
        buffer_write(buf, AUTH_VERSION); // Version
        if (!buffer_can_write(buf)) {
            return false;
        }
        buffer_write(buf, 0x01); // Failure
    }
    return true;
}


static enum auth_parser_state parse_version(auth_parser_t *parser, uint8_t c) {
    if (c != AUTH_VERSION) {
        return AUTH_PARSER_ERROR;
    }
    return AUTH_PARSER_USERNAME_LENGTH;
}

static enum auth_parser_state parse_username_length(auth_parser_t *parser, uint8_t c) {
    if (c == 0) {
        return AUTH_PARSER_PASSWORD_LENGTH; // No username, skip to password length
    }
    parser->total_bytes_to_read = c;
    return AUTH_PARSER_USERNAME;
}

static enum auth_parser_state parse_username(auth_parser_t *parser, uint8_t c) {
    parser->username[parser->bytes_read++] = c;
    if(parser->bytes_read == parser->total_bytes_to_read) {
        parser->username[parser->bytes_read] = '\0';
        parser->bytes_read = 0; // Reset for password reading
        return AUTH_PARSER_PASSWORD_LENGTH;
    }
    return AUTH_PARSER_USERNAME;
}

static enum auth_parser_state parse_password_length(auth_parser_t *parser, uint8_t c) {
    if (c == 0) {
        return AUTH_PARSER_DONE; // No password, finish parsing
    }
    parser->total_bytes_to_read = c;
    return AUTH_PARSER_PASSWORD;
}

static enum auth_parser_state parse_password(auth_parser_t *parser, uint8_t c) {
    parser->password[parser->bytes_read++] = c;
    if(parser->bytes_read == parser->total_bytes_to_read) {
        parser->password[parser->bytes_read] = '\0';
        return AUTH_PARSER_DONE;
    }
    return AUTH_PARSER_PASSWORD;
}

static enum auth_parser_state parse_done(auth_parser_t *parser, uint8_t c) {
    return AUTH_PARSER_DONE;
}

static enum auth_parser_state parse_error(auth_parser_t *parser, uint8_t c) {
    return AUTH_PARSER_ERROR;
}

void try_to_authenticate(auth_parser_t *parser) {
    if (parser == NULL || !auth_parser_is_done(parser) || auth_parser_has_error(parser)) {
        return;
    }
    parser->authenticated = users_login(parser->username, parser->password);
    parser->is_admin = parser->authenticated && users_is_admin(parser->username);
}

bool auth_parser_is_authenticated(const auth_parser_t *parser) {
    return parser != NULL && parser->authenticated;
}
