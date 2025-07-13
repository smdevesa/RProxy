#ifndef USERS_H
#define USERS_H

#include <stdbool.h>

// Maximum lengths for username and password
#define MAX_USERNAME_LENGTH 64
#define MAX_PASSWORD_LENGTH 64

// Maximum number of users that can be stored
#define MAX_USERS 128

// Default admin user.
#define DEFAULT_ADMIN_USERNAME "admin"
#define DEFAULT_ADMIN_PASSWORD "1234"

struct user_t {
    char name[MAX_USERNAME_LENGTH + 1]; // +1 for null terminated
    char pass[MAX_PASSWORD_LENGTH + 1]; // +1 for null terminated
    bool is_admin;
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


#endif //USERS_H
