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

/** * @brief Initialize the configuration module.
  *
  * This function initializes the configuration module, setting up necessary resources.
  * It should be called before any other configuration functions are used.
  *
  * @return true if initialization was successful, false otherwise.
  */
bool config_init();

/** * @brief Clean up the configuration module.
  *
  * This function cleans up resources used by the configuration module.
  * It should be called when the application is shutting down or when the configuration module is no longer needed.
  */
void config_cleanup();

#endif //RPROXY_CONFIG_H
