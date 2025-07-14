#include "config.h"
#include <pthread.h>

static enum auth_methods default_auth_method = USER_PASS; // Default authentication method is USER_PASS
static pthread_mutex_t default_auth_mutex = PTHREAD_MUTEX_INITIALIZER;

bool set_default_auth_method(enum auth_methods method) {
    if (method != NO_AUTH && method != USER_PASS) {
        return false;
    }
    pthread_mutex_lock(&default_auth_mutex);
    default_auth_method = method;
    pthread_mutex_unlock(&default_auth_mutex);
    return true;
}

enum auth_methods get_default_auth_method() {
    pthread_mutex_lock(&default_auth_mutex);
    enum auth_methods method = default_auth_method;
    pthread_mutex_unlock(&default_auth_mutex);
    return method;
}
