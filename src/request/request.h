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

/**
 * Maneja la escritura de respuestas al cliente durante REQUEST_WRITE.
 * @param key Llave del selector
 * @return Nuevo estado de la SM
 */
unsigned request_write(struct selector_key *key);

/**
 * Gestiona la resolución de DNS. (REQUEST_DNS)
 * @param key Llave del selector
 * @return Nuevo estado de la SM
 */
unsigned request_DNS_completed(struct selector_key *key);

/**
 * Espera hasta que la conexión se establezca.
 * @param key Llave del selector
 * @return Nuevo estado de la FSM
 */
unsigned request_connect(struct selector_key *key);

unsigned request_dns(struct selector_key *key);

#endif // REQUEST_H
