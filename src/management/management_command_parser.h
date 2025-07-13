#ifndef MANAGEMENT_COMMAND_PARSER_H
#define MANAGEMENT_COMMAND_PARSER_H

#include <stdint.h>

#define MANAGEMENT_MAX_ARGS 3
#define MANAGEMENT_MAX_STRING_LEN = 255 //(0xFF)

typedef enum {
    MANAGEMENT_COMMAND_USERS = 0,
    MANAGEMENT_COMMAND_ADD_USER,
    MANAGEMENT_COMMAND_DELETE_USERS,
    MANAGEMENT_COMMAND_CHANGE_PASSWORD,
    MANAGEMENT_COMMAND_STATS,
    MANAGEMENT_COMMAND_CHANGE_ROLE
} management_command;

typedef enum {
    MANAGEMENT_PARSER_PENDING = 0,
    MANAGEMENT_PARSER_READING,
    MANAGEMENT_PARSER_DONE,
    MANAGEMENT_PARSER_ERROR,
} management_command_state;

typedef enum {
    MANAGEMENT_SUCCESS = 0,
    MANAGEMENT_INVALID_COMMAND,
    MANAGEMENT_INVALID_ARGUMENTS,
    MANAGEMENT_INVALID_LENGTH,
} management_status;

typedef union arg_type  {
    uint8_t byte;
    char * string; //Cannot exceed 255
}arg_type;

typedef struct management_command_parser {
    management_command_state state;
    management_command command; // Current command being processed
    management_status status; // Status of the command processing

    uint8_t read_args;

    uint8_t slength; // Lenght to read from a string arg
    uint8_t rlength; // Already read bytes form a string
    TArg args[MANAGEMENT_MAX_ARGS]; // Arguments for the command


} management_command_parser;

#endif //MANAGEMENT_COMMAND_PARSER_H
