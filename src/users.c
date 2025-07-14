#include "users.h"
#include <string.h>
#include <time.h>

static struct user_t users[MAX_USERS];
static int user_count;
static int hash_table[MAX_USERS];  // Cada entrada almacena índice al array users o -1

static bool validate_length(const char * string);


// Inicializar tabla hash con -1 (posición vacía)
static void initialize_hash_table() {
    for (int i = 0; i < MAX_USERS; i++) {
        hash_table[i] = -1;
    }
}

// Función hash simple para strings
static unsigned int hash_function(const char *str) {
    unsigned int hash = 0;
    while (*str) {
        hash = (hash * 31) + (*str++);
    }
    return hash % MAX_USERS;
}

// Buscar usuario por nombre (retorna índice o -1)
static int find_user(const char *username) {
    unsigned int index = hash_function(username);
    unsigned int original = index;

    // Sondeo lineal para manejar colisiones
    do {
        if (hash_table[index] == -1) {
            return -1;  // No existe
        }

        int user_idx = hash_table[index];
        if (strcmp(users[user_idx].name, username) == 0) {
            return user_idx;
        }

        index = (index + 1) % MAX_USERS;
    } while (index != original);

    return -1;
}

void users_init() {
    user_count = 0;
    initialize_hash_table();
    // Crear usuario administrador por defecto
    create_user(DEFAULT_ADMIN_USERNAME, DEFAULT_ADMIN_PASSWORD, true);
}

bool users_login(const char *username, const char *password) {
    int user_idx = find_user(username);
    if (user_idx == -1) {
        return false;  // Usuario no encontrado
    }
    return strcmp(users[user_idx].pass, password) == 0;
}

bool create_user(const char *username, const char *password, bool is_admin){
    if (user_count >= MAX_USERS) {
        return false;  // No hay espacio para más usuarios
    }
    if (!validate_length(username) || !validate_length(password)) {
        return false;  // Longitud inválida
    }
    if (find_user(username) != -1) {
        return false;  // Usuario ya existe
    }
    //Crear el nuevo usuario
    struct user_t new_user;
    strncpy(new_user.name, username, MAX_USERNAME_LENGTH);
    new_user.name[MAX_USERNAME_LENGTH] = '\0'; // Asegurar terminación nula
    strncpy(new_user.pass, password, MAX_PASSWORD_LENGTH);
    new_user.pass[MAX_PASSWORD_LENGTH] = '\0'; // Asegurar terminación nula
    new_user.is_admin = is_admin;

    //Agregar el usuario al array
    users[user_count] = new_user;
    // Insertar en la tabla hash
    unsigned int index = hash_function(username);
    while (hash_table[index] != -1) {
        index = (index + 1) % MAX_USERS;  // Manejo de colisiones
    }
    hash_table[index] = user_count;  // Guardar índice del usuario
    user_count++;

    return true;  // Usuario creado exitosamente
}

bool delete_user(const char *username) {
    int user_idx = find_user(username);
    if (user_idx == -1) {
        return false;  // Usuario no encontrado
    }

    // Eliminar usuario original de la tabla hash
    unsigned int index = hash_function(users[user_idx].name);
    while (hash_table[index] != -1) {
        if (hash_table[index] == user_idx) {
            hash_table[index] = -1;
            break;
        }
        index = (index + 1) % MAX_USERS;
    }

    // Si no es el último, hacer swap con el último usuario
    if (user_idx != user_count - 1) {
        char moved_user_name[MAX_USERNAME_LENGTH + 1];
        strncpy(moved_user_name, users[user_count - 1].name, MAX_USERNAME_LENGTH);
        moved_user_name[MAX_USERNAME_LENGTH] = '\0';

        users[user_idx] = users[user_count - 1];  // Hacer swap

        // Buscar y actualizar la entrada vieja del usuario movido
        unsigned int old_index = hash_function(moved_user_name);
        while (hash_table[old_index] != user_count - 1) {
            old_index = (old_index + 1) % MAX_USERS;
        }
        hash_table[old_index] = user_idx;
    }

    user_count--;
    return true;
}

static bool validate_length(const char *string) {
    int length = strlen(string);
    return length <= MAX_USERNAME_LENGTH && length > 0;
}


bool users_is_admin(const char *username) {
    int user_idx = find_user(username);
    if (user_idx == -1) {
        return false;  // Usuario no encontrado
    }
    return users[user_idx].is_admin;
}

bool exists_user(const char *username) {
    return find_user(username) != -1;
}

size_t users_dump_usernames(uint8_t *dst, size_t dst_len) {
    size_t written = 0;

    for (int i = 0; i < user_count; i++) {
        const char *name = users[i].name;
        size_t len = strlen(name);

        if (written + len + 1 > dst_len) {
            break;
        }

        memcpy(dst + written, name, len);
        written += len;

        dst[written++] = '\n'; // separador
    }

    return written;
}

size_t users_get_count() {
    return user_count;
}

bool change_user_password(const char *username, const char *new_password) {
    int user_idx = find_user(username);
    if (user_idx == -1) {
        return false;
    }
    if (!validate_length(new_password)) {
        return false;
    }
    strncpy(users[user_idx].pass, new_password, MAX_PASSWORD_LENGTH);
    users[user_idx].pass[MAX_PASSWORD_LENGTH] = '\0';
    return true;
}

bool change_user_role(const char *username, bool is_admin) {
    int user_idx = find_user(username);
    if (user_idx == -1) {
        return false;
    }
    users[user_idx].is_admin = is_admin;
    return true;
}

bool register_user_access(const char *username, const char *ip_or_site) {
    int user_idx = find_user(username);
    if (user_idx == -1) {
        return false;  // Usuario no encontrado
    }

    struct user_t *user = &users[user_idx];
    if (user->access_log_count >= MAX_ACCESS_LOGS) {
        // Si el log está lleno, sobrescribir el más antiguo
        user->current_access_log_index = (user->current_access_log_index + 1) % MAX_ACCESS_LOGS;
    } else {
        user->access_log_count++;
    }

    struct access_log_t *log = &user->access_logs[user->current_access_log_index];
    strncpy(log->ip_or_site, ip_or_site, MAX_IP_OR_SITE_LENGTH);
    log->ip_or_site[MAX_IP_OR_SITE_LENGTH] = '\0'; // Asegurar terminación nula
    log->timestamp = time(NULL); // Registrar tiempo actual

    return true;
}

size_t get_user_access_history(const char *username, struct access_log_t *logs, size_t max_logs){

    int user_idx = find_user(username);
    if (user_idx == -1) {
        return 0;  // Usuario no encontrado
    }

    struct user_t *user = &users[user_idx];
    size_t count = user->access_log_count < max_logs ? user->access_log_count : max_logs;

    for (size_t i = 0; i < count; i++) {
        logs[i] = user->access_logs[(user->current_access_log_index + i) % MAX_ACCESS_LOGS];
    }

    return count;
}
