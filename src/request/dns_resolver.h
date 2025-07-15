#ifndef DNS_RESOLVER_H
#define DNS_RESOLVER_H

#include <netdb.h>
#include <stdbool.h>
#include "../selector.h"


/**
*
* Initializes the DNS resolver for the given key.
* @param key Pointer to the selector key containing the client data.
*
*/
void dns_resolver_init(struct selector_key *key);


/**
 *
 *  Cancels the DNS resolution process for the given key.
 *
 */
void dns_resolution_cancel(struct selector_key *key);


/**
 * Callback function to be called when DNS resolution is done.
 * This function will be called by the selector when the DNS resolution is complete.
 * @param sv Union containing the selector key.
 */
void dns_resolution_done(union sigval sv);

#endif /* DNS_RESOLVER_H */
