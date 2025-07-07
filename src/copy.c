#include "copy.h"

static unsigned read_from_client(struct selector_key *key, struct client_data *data);
static unsigned read_from_origin(struct selector_key *key, struct client_data *data);
static unsigned write_from_client(struct selector_key *key, struct client_data *data);
static unsigned write_from_origin(struct selector_key *key, struct client_data *data);

void copy_init(unsigned int state, struct selector_key *key) {
    printf("Initializing copy state...\n");
    struct client_data *data = ATTACHMENT(key);

    printf("Setting client fd= %d and origin fd = %d\n", data->client_fd, data->origin_fd);
    if (selector_set_interest(key->s, data->client_fd, OP_READ) != SELECTOR_SUCCESS) {
        close_connection(key);
        return;
    }

    if (selector_set_interest(key->s, data->origin_fd, OP_READ) != SELECTOR_SUCCESS) {
        close_connection(key);
        return;
    }

}

unsigned copy_read(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    //Check who is sending data
    if (key->fd == data->client_fd) {
        printf("Copying from client to origin...\n");
        return read_from_client(key, data);
    } else if (key->fd == data->origin_fd) {
        printf("Copying from origin to client...\n");
        return read_from_origin(key, data);
    } else {
        return ERROR; // Invalid file descriptor
    }
}

unsigned copy_write(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);

    if (key->fd == data->client_fd) {
        printf("Writing to client from origin...\n");
        return write_from_origin(key, data);
    } else if (key->fd == data->origin_fd) {
        printf("Writing to origin from client...\n");
        return write_from_client(key, data);
    } else {
        return ERROR;
    }
}


static unsigned read_from_client(struct selector_key *key, struct client_data *data) {
    printf("Reading from client...\n");
    size_t read_limit;

    if (!buffer_can_write(&data->origin_buffer)) {
        return COPY;
    }

    uint8_t *read_buffer = buffer_write_ptr(&data->origin_buffer, &read_limit);
    ssize_t read_count = recv(key->fd, read_buffer, read_limit, 0);

    if(read_count < 0) {
        perror("recv from client failed");
        return ERROR;
    } else if(read_count == 0) {
        printf("Cliente cerró la conexión normalmente\n");
        return DONE;  // o un estado específico para manejar el cierre
    }

    buffer_write_adv(&data->origin_buffer, read_count);

    printf("Prepare to write to origin...\n");
    if (selector_set_interest(key->s, data->origin_fd, OP_WRITE) != SELECTOR_SUCCESS) {
        return ERROR;
    }
    return COPY;
}

static unsigned read_from_origin(struct selector_key *key, struct client_data *data) {
    printf("Reading from origin...\n");
    size_t read_limit;      // Maximum bytes to read in this operation
    ssize_t read_count;      // Total bytes read in this operation

    if (!buffer_can_write(&data->client_buffer)) {
        return COPY;
    }

    uint8_t *read_buffer = buffer_write_ptr(&data->client_buffer, &read_limit);
    read_count = recv(key->fd, read_buffer, read_limit, 0);

    if(read_count < 0) {
        perror("recv from client failed");
        return ERROR;
    } else if(read_count == 0) {
        printf("No se leen más bytes\n");
        return DONE;
    }

    buffer_write_adv(&data->client_buffer, read_count);

    printf("Prepare to write to client...\n");
    if (selector_set_interest(key->s, data->client_fd, OP_WRITE) != SELECTOR_SUCCESS) {
        return ERROR;
    }
    return COPY; // Continue copying from origin to client
}

static unsigned write_from_client(struct selector_key *key, struct client_data *data){
    printf("Writing to origin from client...\n");
    size_t write_limit;
    ssize_t write_count;

    if (!buffer_can_read(&data->origin_buffer)){
        return COPY;  // No hay datos para escribir
    }

    uint8_t *write_buffer = buffer_read_ptr(&data->origin_buffer, &write_limit);
    write_count = send(key->fd, write_buffer, write_limit, MSG_NOSIGNAL);

    if (write_count <= 0) {
        perror("send");
        return ERROR;
    }

    buffer_read_adv(&data->origin_buffer, write_count);

    if (!buffer_can_read(&data->origin_buffer)){
        // Si no hay más datos para escribir, volver a modo lectura
        if (selector_set_interest(key->s, data->origin_fd, OP_READ) != SELECTOR_SUCCESS){
            return ERROR;
        }
    }

    return COPY;
}

static unsigned write_from_origin(struct selector_key *key, struct client_data *data){
    printf("Writing to client from origin...\n");
    size_t write_limit;
    ssize_t write_count;

    if (!buffer_can_read(&data->client_buffer)){
        return COPY;  // No hay datos para escribir
    }

    uint8_t *write_buffer = buffer_read_ptr(&data->client_buffer, &write_limit);
    write_count = send(key->fd, write_buffer, write_limit, MSG_NOSIGNAL);

    if (write_count <= 0) {
        perror("send");
        printf("Me falló send\n");
        return ERROR;
    }

    buffer_read_adv(&data->client_buffer, write_count);

    if (!buffer_can_read(&data->client_buffer)){
        // Si no hay más datos para escribir, volver a modo lectura
        if (selector_set_interest(key->s, data->client_fd, OP_READ) != SELECTOR_SUCCESS){
            printf("Me falló selector_set_interest\n");
            return ERROR;
        }
    }

    return COPY;
}

void copy_close(unsigned int state, struct selector_key *key) {
    printf("closing copy...\n");
}