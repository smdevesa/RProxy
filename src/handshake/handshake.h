//
// Created by Santiago Devesa on 02/07/2025.
//

#ifndef RPROXY_HANDSHAKE_H
#define RPROXY_HANDSHAKE_H

#include "handshake_parser.h"
#include "../selector.h"


/**
 * Initializes the handshake read state.
 * @param state The initial state for the handshake read.
 * @param key The selector key containing the client data.
 */
void handshake_read_init(unsigned state, struct selector_key *key);

/**
 * Reads handshake data from the client.
 *
 * @param key The selector key containing the client data.
 * @return The next state to transition to.
 */
unsigned handshake_read(struct selector_key *key);

/**
 * Writes handshake response to the client.
 *
 * @param key The selector key containing the client data.
 * @return The next state to transition to.
 */
unsigned handshake_write(struct selector_key *key);

#endif //RPROXY_HANDSHAKE_H
