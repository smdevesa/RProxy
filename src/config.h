#ifndef RPROXY_CONFIG_H
#define RPROXY_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "handshake/handshake_parser.h"

/** * @brief Get the default authentication method.
  *
  */
enum auth_methods get_default_auth_method();

/** * @brief Set the default authentication method.
  *
  * @param method The authentication method to set as default. Accepts values: NO_AUTH (0x00), USERNAME_PASSWORD (0x02)
  * @return true if the method was set successfully, false otherwise.
  */
bool set_default_auth_method(enum auth_methods method);

#endif //RPROXY_CONFIG_H
