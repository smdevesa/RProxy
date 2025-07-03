#ifndef RPROXY_AUTH_H
#define RPROXY_AUTH_H

#include "../selector.h"
#include "auth_parser.h"

void auth_read_init(unsigned state, struct selector_key *key);
unsigned auth_read(struct selector_key *key);
unsigned auth_write(struct selector_key *key);

#endif //RPROXY_AUTH_H
