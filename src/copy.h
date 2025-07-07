#ifndef COPY_H
#define COPY_H

#include "selector.h"
#include "socks5/socks5.h"
#include <stdio.h>

void copy_init(unsigned int state, struct selector_key *key);
unsigned copy_read(struct selector_key *key);
unsigned copy_write(struct selector_key *key);
void copy_close(unsigned int state, struct selector_key *key);

#endif //COPY_H
