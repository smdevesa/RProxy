#ifndef DNS_RESOLVER_H
#define DNS_RESOLVER_H

#include <netdb.h>
#include <stdbool.h>
#include "../selector.h"

void dns_resolver_init(struct selector_key *key);
void dns_resolution_cancel(struct selector_key *key);

void dns_resolution_done(union sigval sv);

#endif /* DNS_RESOLVER_H */
