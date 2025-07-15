#ifndef RPROXY_AUTH_H
#define RPROXY_AUTH_H

#include "../selector.h"
#include "auth_parser.h"

/**
 * Initializes the authentication read state.
 * @param state The initial state for the authentication read.
 * @param key The selector key containing the client data.
 */
void auth_read_init(unsigned state, struct selector_key *key);

/**
 * Reads authentication data from the client.
 *
 * @param key The selector key containing the client data.
 * @return The next state to transition to.
 */
unsigned auth_read(struct selector_key *key);

/**
 * Writes authentication response to the client.
 *
 * @param key The selector key containing the client data.
 * @return The next state to transition to.
 */
unsigned auth_write(struct selector_key *key);

#endif //RPROXY_AUTH_H
