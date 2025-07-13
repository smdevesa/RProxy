#include "management_command_parser.h"

#define DELIMITER ':'

typedef management_command_state (*state_handler)(management_command_parser *, uint8_t);

static management_command_state parse_version(management_command_parser *parser, uint8_t c);
static management_command_state parse_command(management_command_parser *parser, uint8_t c);
static management_command_state parse_payload_len(management_command_parser *parser, uint8_t c);
static management_command_state parse_payload(management_command_parser *parser, uint8_t c);
static management_command_state parse_done(management_command_parser *parser, uint8_t c);
static management_command_state parse_error(management_command_parser *parser, uint8_t c);

static const uint8_t command_args_count[] = {
        0,  /* MANAGEMENT_COMMAND_USERS */
        2,  /* MANAGEMENT_COMMAND_ADD_USER */
        1,  /* MANAGEMENT_COMMAND_DELETE_USERS */
        2,  /* MANAGEMENT_COMMAND_CHANGE_PASSWORD */
        0,  /* MANAGEMENT_COMMAND_STATS */
        2   /* MANAGEMENT_COMMAND_CHANGE_ROLE */
};

static state_handler state_handlers[] = {
    (state_handler)parse_version,
    (state_handler)parse_command,
    (state_handler)parse_payload_len,
    (state_handler)parse_payload,
    (state_handler)parse_done,
    (state_handler)parse_error
};

void management_command_parser_init(management_command_parser *parser) {
    if (parser == NULL) return;
    parser->state = MANAGEMENT_PARSER_VERSION;
    parser->read_args = 0;
    parser->to_read_len = 0;
    parser->read_len = 0;
}

management_command_state management_command_parser_parse(management_command_parser *parser, struct buffer *buf){
    while (buffer_can_read(buf) && !management_command_parser_is_done(parser)) {
        uint8_t c = buffer_read(buf);
        parser->state = state_handlers[parser->state](parser, c);
    }
    return parser->state;
}

bool management_command_parser_is_done(const management_command_parser *parser) {
    return parser != NULL && (parser->state == MANAGEMENT_PARSER_DONE || parser->state == MANAGEMENT_PARSER_ERROR);
}

bool management_command_parser_has_error(const management_command_parser *parser) {
    return parser != NULL && parser->state == MANAGEMENT_PARSER_ERROR;
}

bool management_parser_build_response(const management_command_parser *parser, struct buffer *buf, management_status reply_code) {
    if (parser == NULL || buf == NULL) return false;

    // Build the response based on the parser state
    uint8_t response[] = {0x01, reply_code}; // Version 1, followed by the status code
    size_t response_length = sizeof(response) / sizeof(response[0]);

    for (size_t i = 0; i < response_length; ++i) {
        if (!buffer_can_write(buf)) {
            return false;
        }
        buffer_write(buf, response[i]);
    }
    return true;
}

static management_command_state parse_version(management_command_parser *parser, uint8_t c){
    if (c != MANAGEMENT_VERSION) {
        parser->status = MANAGEMENT_INVALID_VERSION;
        return MANAGEMENT_PARSER_ERROR;
    }
    return MANAGEMENT_PARSER_COMMAND;
}

static management_command_state parse_command(management_command_parser *parser, uint8_t c) {
 if (c < MANAGEMENT_COMMAND_MIN || c > MANAGEMENT_COMMAND_MAX) {
    parser->status = MANAGEMENT_INVALID_COMMAND;
     return MANAGEMENT_PARSER_ERROR;
    }
    parser->command = (management_command)c;
    return MANAGEMENT_PARSER_PAYLOAD_LEN;
}

static management_command_state parse_payload_len(management_command_parser *parser, uint8_t c) {
    if (c > MANAGEMENT_MAX_STRING_LEN) {
        parser->status = MANAGEMENT_INVALID_LENGTH;
        return MANAGEMENT_PARSER_ERROR;
    }
    parser->to_read_len = c;
    return MANAGEMENT_PARSER_PAYLOAD;
}

static management_command_state parse_payload(management_command_parser *parser, uint8_t c) {
    if (parser->to_read_len == 0) {
        if (parser->read_len > 0) {
            parser->args[parser->read_args][parser->read_len] = '\0';
            parser->read_args++;
        }

        if (parser->read_args != command_args_count[parser->command]) {
            parser->status = MANAGEMENT_INVALID_ARGUMENTS;
            return MANAGEMENT_PARSER_ERROR;
        }

        parser->status = MANAGEMENT_SUCCESS;
        return MANAGEMENT_PARSER_DONE;
    }

    if (c == DELIMITER) {
        if (parser->read_len == 0) {
            parser->status = MANAGEMENT_INVALID_ARGUMENTS;
            return MANAGEMENT_PARSER_ERROR;
        }

        parser->args[parser->read_args][parser->read_len] = '\0';
        parser->read_args++;
        parser->read_len = 0;
        parser->to_read_len--;
        return MANAGEMENT_PARSER_PAYLOAD;
    }

    if (parser->read_len >= (MANAGEMENT_MAX_STRING_LEN - 1)) {
        parser->status = MANAGEMENT_INVALID_ARGUMENTS;
        return MANAGEMENT_PARSER_ERROR;
    }

    parser->args[parser->read_args][parser->read_len++] = c;
    parser->to_read_len--;
    return MANAGEMENT_PARSER_PAYLOAD;
}

static management_command_state parse_done(management_command_parser *parser, uint8_t c) {
    return MANAGEMENT_PARSER_DONE;
}

static management_command_state parse_error(management_command_parser *parser, uint8_t c) {
    return MANAGEMENT_PARSER_ERROR;
}
