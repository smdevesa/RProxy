//
// Created by Santiago Devesa on 02/07/2025.
//

#ifndef RPROXY_HANDSHAKE_H
#define RPROXY_HANDSHAKE_H

#include "handshake_parser.h"
#include "../selector.h"

void handshake_read_init(unsigned state, struct selector_key *key);

unsigned handshake_read(struct selector_key *key);

unsigned handshake_write(struct selector_key *key);

#endif //RPROXY_HANDSHAKE_H
