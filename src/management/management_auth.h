#ifndef MANAGEMENT_AUTH_H
#define MANAGEMENT_AUTH_H

#include "../selector.h"

/*
 *
 *       AUTHENTICATION REQUEST  (RFC 1929)
 *
 *              REQUEST
 *          CLIENT->SERVER
 *      ------------------------------------------------
 *      | VERSION | USR-LEN |  USR  |  PSW-LEN |  PSW  |
 *      ------------------------------------------------
 *      |    1    |    1    | 0-255 |    1     | 0-255 |
 *
 *
 *          AUTHENTICATION  RESPONSE
 *               SERVER->CLIENT
 *          --------------------
 *          | VERSION | STATUS |
 *          --------------------
 *          |    1    |    1   |
 *
 *
 */


/**
 * Initializes the management authentication read state.
 * @param state The initial state for the management authentication read.
 * @param key The selector key containing the client data.
 */

void management_auth_init(unsigned int state, struct selector_key *key);

/**
 * Reads management authentication data from the client.
 *
 * @param key The selector key containing the client data.
 * @return The next state to transition to.
 */
unsigned management_auth_read(struct selector_key *key);

/**
 * Writes management authentication response to the client.
 *
 * @param key The selector key containing the client data.
 * @return The next state to transition to.
 */
unsigned management_auth_write(struct selector_key *key);

#endif //MANAGEMENT_AUTH_H
