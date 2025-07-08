#ifndef REQUEST_PARSER_H
#define REQUEST_PARSER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../buffer.h"

#define REQUEST_MAX_ADDR_LENGTH 256
#define IPV4_LENGTH 4
#define IPV6_LENGTH 16


typedef enum request_parser_state {
    REQUEST_PARSER_VERSION,          // Reading the version byte (must be 0x05)
    REQUEST_PARSER_COMMAND,          // Reading the command byte (e.g., CONNECT, BIND)
    REQUEST_PARSER_RSV,              // Reading the reserved byte (should be 0x00)
    REQUEST_PARSER_ATYP,             // Reading the address type
    REQUEST_PARSER_DST_ADDR,         // Reading the destination address
    REQUEST_PARSER_DST_PORT,         // Reading the destination port
    REQUEST_PARSER_DONE,
    REQUEST_PARSER_ERROR
} request_parser_state;

typedef enum request_command {
    //Solo soportaremos CONNECT
    REQUEST_COMMAND_CONNECT = 0x01,
    REQUEST_COMMAND_BIND = 0x02,
    REQUEST_COMMAND_UDP = 0x03
} request_command;

typedef enum address_type {
    ADDRESS_TYPE_IPV4 = 0x01,
    ADDRESS_TYPE_DOMAIN = 0x03,
    ADDRESS_TYPE_IPV6 = 0x04
} address_type;

typedef enum request_reply {
    REQUEST_REPLY_SUCCESS = 0,
    REQUEST_REPLY_FAILURE,
    REQUEST_REPLY_NOT_ALLOWED,
    REQUEST_REPLY_NETWORK_UNREACHABLE,
    REQUEST_REPLY_HOST_UNREACHABLE,
    REQUEST_REPLY_CONNECTION_REFUSED,
    REQUEST_REPLY_TTL_EXPIRED,
    REQUEST_REPLY_COMMAND_NOT_SUPPORTED,
    REQUEST_REPLY_ADDRESS_TYPE_NOT_SUPPORTED
} request_reply;

typedef struct request_parser {
    request_parser_state state;           // Current state of the request parser
    request_command command;              // The command being processed
    address_type address_type;            // The type of address being processed
    size_t dst_addr_length;               // Length of the destination address
    size_t dst_port;                      // Destination port number
    uint8_t dst_addr[REQUEST_MAX_ADDR_LENGTH]; // Buffer to hold destination address (IPv4/6/domain) --> Ipv4 is 4 bytes, IPv6 is 16 bytes, domain can be up to 255 bytes
    uint8_t bytes_read;                   // Internal counter for parsing
} request_parser_t;

void request_parser_init(request_parser_t *parser);
request_parser_state request_parser_consume(request_parser_t *parser, struct buffer *b);
bool request_parser_is_done(const request_parser_t *parser);
bool request_parser_has_error(const request_parser_t *parser);
bool request_build_response(const request_parser_t *parser, struct buffer *buf, request_reply reply_code);

#endif //REQUEST_PARSER_H
