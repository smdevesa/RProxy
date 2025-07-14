#ifndef RPROXY_MANAGEMENT_COMMAND_H
#define RPROXY_MANAGEMENT_COMMAND_H

#include "../selector.h"

/**@brief Initializes the management command read state.
 * This function sets up the initial state for reading management commands.
 * @param key Pointer to the selector key containing the connection information.
 */
void management_command_read_init(struct selector_key *key);

/**
 * @brief Handles the management command read event.
 * This function processes the management command read event, parsing the command
 * and executing it if valid.
 * @param key Pointer to the selector key containing the connection information.
 * @return Returns 0 on success, or a negative value on error.
 */
int management_command_read(struct selector_key *key);

/**
 * @brief Handles the management command write event.
 * This function processes the management command write event, sending the response
 * back to the client.
 * @param key Pointer to the selector key containing the connection information.
 * @return Returns 0 on success, or a negative value on error.
 */
int management_command_write(struct selector_key *key);

#endif //RPROXY_MANAGEMENT_COMMAND_H
