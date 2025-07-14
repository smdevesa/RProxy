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

void management_auth_init(unsigned int state, struct selector_key *key);
unsigned management_auth_read(struct selector_key *key);
unsigned management_auth_write(struct selector_key *key);

#endif //MANAGEMENT_AUTH_H
