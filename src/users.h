#ifndef USERS_H
#define USERS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Maximum lengths for username and password
#define MAX_USERNAME_LENGTH 64
#define MAX_PASSWORD_LENGTH 64

// Maximum number of users that can be stored
#define MAX_USERS 128
#define MAX_ACCESS_LOGS 100
#define MAX_IP_OR_SITE_LENGTH 64

// Default admin user.
#define DEFAULT_ADMIN_USERNAME "admin"
#define DEFAULT_ADMIN_PASSWORD "1234"

struct access_log_t {
    char ip_or_site[MAX_IP_OR_SITE_LENGTH + 1]; // +1 for null terminated
    uint32_t timestamp; // Timestamp of the action
};

struct user_t {
    char name[MAX_USERNAME_LENGTH + 1]; // +1 for null terminated
    char pass[MAX_PASSWORD_LENGTH + 1]; // +1 for null terminated
    bool is_admin;

    //Registro de accesos
    struct access_log_t access_logs[MAX_ACCESS_LOGS];
    size_t access_log_count; // Number of access logs
    size_t current_access_log_index; // Current index for adding new logs
};

//Initialize user system
void users_init();

/**
 * Login a user with the given username and password. Validation included
 *
 */
bool users_login(const char *username, const char *password);

/**
 * Adds a new user with the given username and password.
 * Returns true if the user was added successfully, false otherwise.
 */
bool create_user(const char *username, const char *password, bool is_admin);

/**
 * Deletes a user with the given username.
 */
bool delete_user(const char *username);


/** checks if the user exists and is an admin
 * Returns true if the user is an admin, false otherwise.
 */
bool users_is_admin(const char *username);

/**
 * Checks if a user with the given username exists.
 * Returns true if the user exists, false otherwise.
 */
bool exists_user(const char *username);

/**
 * Dumps the usernames of all users into the provided buffer.
 * Returns the number of usernames written to the buffer.
 */
size_t users_dump_usernames(uint8_t *dst, size_t dst_len);

/**
 * @return The number of users currently stored in the system.
 */
size_t users_get_count();

/**
 * Changes the password of the user with the given username.
 * Returns true if the password was changed successfully, false otherwise.
 */
bool change_user_password(const char *username, const char *new_password);

/**
 * Changes the role of the user with the given username.
 * If is_admin is true, the user will be set as an admin, otherwise as a regular user.
 * Returns true if the role was changed successfully, false otherwise.
 */
bool change_user_role(const char *username, bool is_admin);

/**
 * Registra un nuevo acceso para el usuario especificado
 * @param username Nombre del usuario
 * @param ip_or_site IP o sitio al que accedió
 * @return true si se registró correctamente, false en caso contrario
 */
bool register_user_access(const char *username, const char *ip_or_site);

/**
 * Obtiene el historial de accesos de un usuario
 * @param username Nombre del usuario
 * @param logs Buffer donde se copiarán los logs
 * @param max_logs Tamaño máximo del buffer
 * @return Número de registros copiados
 */
size_t get_user_access_history(const char *username, struct access_log_t *logs, size_t max_logs);

#endif //USERS_H
