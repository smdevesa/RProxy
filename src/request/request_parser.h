//
// Created by jrambau on 03/07/25.
//

#ifndef REQUEST_PARSER_H
#define REQUEST_PARSER_H

#include <stddef.h>

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
    REQUEST_REPLY_SUCCESS,
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
    request_parser_state state;   // Current state of the request parser
    request_command command;      // The command being processed
    address_type address_type;    // The type of address being processed
    size_t dst_addr_length;            // Length of the destination address
    size_t dst_port;                   // Destination port number
} request_parser_t;

#endif //REQUEST_PARSER_H
