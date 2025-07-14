#include "config.h"
#include <pthread.h>

static enum auth_methods default_auth_method = USER_PASS;
static pthread_rwlock_t default_auth_rwlock;

bool config_init(void) {
    if (pthread_rwlock_init(&default_auth_rwlock, NULL) != 0) {
        return false;
    }
    default_auth_method = USER_PASS; // o lo que quieras por defecto
    return true;
}

void config_cleanup(void) {
    pthread_rwlock_destroy(&default_auth_rwlock);
}

bool set_default_auth_method(enum auth_methods method) {
    if (method != NO_AUTH && method != USER_PASS) {
        return false;
    }

    if (pthread_rwlock_wrlock(&default_auth_rwlock) != 0) {
        return false;
    }

    default_auth_method = method;

    pthread_rwlock_unlock(&default_auth_rwlock);
    return true;
}

enum auth_methods get_default_auth_method() {
    enum auth_methods method;

    if (pthread_rwlock_rdlock(&default_auth_rwlock) != 0) {
        return USER_PASS; // fallback
    }

    method = default_auth_method;

    pthread_rwlock_unlock(&default_auth_rwlock);
    return method;
}
