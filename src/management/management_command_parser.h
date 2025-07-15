#ifndef MANAGEMENT_COMMAND_PARSER_H
#define MANAGEMENT_COMMAND_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include "../buffer.h"

#define MANAGEMENT_MAX_ARGS 3
#define MANAGEMENT_MAX_STRING_LEN 0xFF //(0xFF)
#define MANAGEMENT_VERSION 0x01 // Version of the management protocol

typedef enum {
    MANAGEMENT_COMMAND_USERS = 0,
    MANAGEMENT_COMMAND_ADD_USER,
    MANAGEMENT_COMMAND_DELETE_USERS,
    MANAGEMENT_COMMAND_CHANGE_PASSWORD,
    MANAGEMENT_COMMAND_STATS,
    MANAGEMENT_COMMAND_CHANGE_ROLE,
    MANAGEMENT_COMMAND_SET_DEFAULT_AUTH_METHOD,
    MANAGEMENT_COMMAND_GET_DEFAULT_AUTH_METHOD,
    MANAGEMENT_COMMAND_VIEW_ACTIVITY_LOG
} management_command;

#define MANAGEMENT_COMMAND_MIN MANAGEMENT_COMMAND_USERS // Minimum command value
#define MANAGEMENT_COMMAND_MAX MANAGEMENT_COMMAND_VIEW_ACTIVITY_LOG // Maximum command value

typedef enum {
    MANAGEMENT_PARSER_VERSION = 0,
    MANAGEMENT_PARSER_COMMAND,
    MANAGEMENT_PARSER_PAYLOAD_LEN,
    MANAGEMENT_PARSER_PAYLOAD,
    MANAGEMENT_PARSER_DONE,
    MANAGEMENT_PARSER_ERROR,
} management_command_state;

typedef enum {
    MANAGEMENT_SUCCESS = 0,
    MANAGEMENT_FORBIDDEN,
    MANAGEMENT_INVALID_VERSION,
    MANAGEMENT_INVALID_COMMAND,
    MANAGEMENT_INVALID_ARGUMENTS,
    MANAGEMENT_USER_ALREADY_EXISTS,
    MANAGEMENT_USER_NOT_FOUND,
    MANAGEMENT_INVALID_ROLE,
    MANAGEMENT_INVALID_LENGTH,
    MANAGEMENT_SERVER_ERROR,
} management_status;

typedef struct management_command_parser {
    management_command_state state;
    management_command command; // Current command being processed
    management_status status; // Status of the command processing
    uint8_t read_args; // Number of arguments read so far

    uint8_t read_len; // Length of the current argument being read
    uint8_t to_read_len; // Pending chars to read
    uint8_t args[MANAGEMENT_MAX_ARGS][MANAGEMENT_MAX_STRING_LEN + 1]; // Arguments for the command
} management_command_parser;

/**
 * @brief Initializes the management command parser.
 *
 * @param parser Pointer to the management command parser structure.
 */
void management_command_parser_init(management_command_parser *parser);

/**
 * @brief Parses the management command from the given buffer.
 *
 * @param parser Pointer to the management command parser structure.
 * @param buf Pointer to the buffer containing the command data.
 * @return The current state of the parser after processing the command.
 */
management_command_state management_command_parser_parse(management_command_parser *parser, struct buffer *buf);


/**
 * @brief Builds a response based on the parser's state and reply code.
 *
 * @param parser Pointer to the management command parser structure.
 * @param buf Pointer to the buffer where the response will be written.
 * @param reply_code The status code to include in the response.
 * @param msg The message to include in the response.
 * @return true if the response was successfully built, false otherwise.
 */
bool management_command_parser_build_response(const management_command_parser *parser, struct buffer *buf, management_status reply_code, const char *msg);

/**
 * @brief Checks if the parser has completed processing the command.
 *
 * @param parser Pointer to the management command parser structure.
 * @return true if the parser has finished, false otherwise.
 */

bool management_command_parser_is_done(const management_command_parser *parser);

/**
 * @brief Checks if an error occurred during command parsing.
 *
 * @param parser Pointer to the management command parser structure.
 * @return true if an error occurred, false otherwise.
 */

bool management_command_parser_has_error(const management_command_parser *parser);

#endif //MANAGEMENT_COMMAND_PARSER_H
