#ifndef COPY_H
#define COPY_H

#include "selector.h"
#include "socks5/socks5.h"
#include <stdio.h>

/**
 * Initializes the copy state for reading data from the client.
 * @param state The initial state for the copy read.
 * @param key The selector key containing the client data.
 */
void copy_init(unsigned int state, struct selector_key *key);
/**
 * Reads data from the client and writes it to the origin server.
 * @param key The selector key containing the client data.
 * @return The next state to transition to.
 */
unsigned copy_read(struct selector_key *key);
/**
 * Writes data from the origin server to the client.
 * @param key The selector key containing the client data.
 * @return The next state to transition to.
 */
unsigned copy_write(struct selector_key *key);

#endif //COPY_H
