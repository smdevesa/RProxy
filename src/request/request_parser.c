// request_parser.c - Parser para la fase de REQUEST del protocolo SOCKS5

#include "request_parser.h"
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

#define IPV4_LENGTH 4
#define IPV6_LENGTH 16

typedef request_parser_state (*parser_state_fn)(request_parser_t *, uint8_t);

static request_parser_state parse_version(request_parser_t *parser, uint8_t c);
static request_parser_state parse_command(request_parser_t *parser, uint8_t c);
static request_parser_state parse_rsv(request_parser_t *parser, uint8_t c);
static request_parser_state parse_atyp(request_parser_t *parser, uint8_t c);
static request_parser_state parse_dst_addr(request_parser_t *parser, uint8_t c);
static request_parser_state parse_dst_port(request_parser_t *parser, uint8_t c);
static request_parser_state parse_error(request_parser_t *parser, uint8_t c);

static parser_state_fn state_table[] = {
        parse_version,
        parse_command,
        parse_rsv,
        parse_atyp,
        parse_dst_addr,
        parse_dst_port,
        parse_error,
        parse_error
};

void request_parser_init(request_parser_t *parser) {
    if (parser == NULL) return;
    parser->state = REQUEST_PARSER_VERSION;
    parser->dst_port = 0;
    parser->dst_addr_length = 0;
    parser->bytes_read = 0;
}

request_parser_state request_parser_consume(request_parser_t *parser, struct buffer *b) {
    while (buffer_can_read(b) && !request_parser_is_done(parser)) {
        uint8_t c = buffer_read(b);
        parser->state = state_table[parser->state](parser, c);
    }
    return parser->state;
}

bool request_parser_is_done(const request_parser_t *parser) {
    return parser != NULL &&
           (parser->state == REQUEST_PARSER_DONE || parser->state == REQUEST_PARSER_ERROR);
}

bool request_parser_has_error(const request_parser_t *parser) {
    return parser != NULL && parser->state == REQUEST_PARSER_ERROR;
}

static request_parser_state parse_version(request_parser_t *parser, uint8_t c) {
    return (c == 0x05) ? REQUEST_PARSER_COMMAND : REQUEST_PARSER_ERROR;
}

static request_parser_state parse_command(request_parser_t *parser, uint8_t c) {
    if (c == REQUEST_COMMAND_CONNECT) {
        parser->command = c;
        return REQUEST_PARSER_RSV;
    }
    return REQUEST_PARSER_ERROR;
}

static request_parser_state parse_rsv(request_parser_t *parser, uint8_t c) {
    return (c == 0x00) ? REQUEST_PARSER_ATYP : REQUEST_PARSER_ERROR;
}

static request_parser_state parse_atyp(request_parser_t *parser, uint8_t c) {
    parser->address_type = c;
    parser->bytes_read = 0;
    switch (c) {
        case ADDRESS_TYPE_IPV4:
            parser->dst_addr_length = IPV4_LENGTH;
            return REQUEST_PARSER_DST_ADDR;
        case ADDRESS_TYPE_IPV6:
            parser->dst_addr_length = IPV6_LENGTH;
            return REQUEST_PARSER_DST_ADDR;
        case ADDRESS_TYPE_DOMAIN:
            return REQUEST_PARSER_DST_ADDR;
        default:
            return REQUEST_PARSER_ERROR;
    }
}

static request_parser_state parse_dst_addr(request_parser_t *parser, uint8_t c) {
    if (parser->address_type == ADDRESS_TYPE_DOMAIN && parser->bytes_read == 0) {
        parser->dst_addr_length = c;
        parser->bytes_read++;
        return REQUEST_PARSER_DST_ADDR;
    }

    size_t index = parser->bytes_read - (parser->address_type == ADDRESS_TYPE_DOMAIN ? 1 : 0);
    if (index >= sizeof(parser->dst_addr)) return REQUEST_PARSER_ERROR;

    parser->dst_addr[parser->bytes_read++] = c;

    if ((parser->address_type == ADDRESS_TYPE_DOMAIN && parser->bytes_read == parser->dst_addr_length + 1) ||
        (parser->address_type != ADDRESS_TYPE_DOMAIN && parser->bytes_read == parser->dst_addr_length)) {
        parser->bytes_read = 0;
        return REQUEST_PARSER_DST_PORT;
    }

    return REQUEST_PARSER_DST_ADDR;
}

static request_parser_state parse_dst_port(request_parser_t *parser, uint8_t c) {
    parser->dst_port = (parser->dst_port << 8) | c;
    if (++parser->bytes_read == 2) {
        parser->bytes_read = 0;
        return REQUEST_PARSER_DONE;
    }
    return REQUEST_PARSER_DST_PORT;
}

static request_parser_state parse_error(request_parser_t *parser, uint8_t c) {
    (void)parser;
    (void)c;
    return REQUEST_PARSER_ERROR;
}
