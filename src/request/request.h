#ifndef REQUEST_H
#define REQUEST_H

#include "../selector.h"

/**
 * Inicializa el parser cuando se entra al estado REQUEST_READ.
 * @param state Estado previo
 * @param key Llave del selector
 */
void request_read_init(const unsigned state, struct selector_key *key);

/**
 * Maneja los datos entrantes del cliente durante REQUEST_READ.
 * @param key Llave del selector
 * @return Nuevo estado de la FSM
 */
unsigned request_read(struct selector_key *key);

#endif // REQUEST_H
