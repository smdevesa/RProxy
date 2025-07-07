#include "copy.h"

static unsigned copy_from_client(struct selector_key *key, struct client_data *data);
static unsigned copy_from_origin(struct selector_key *key, struct client_data *data);

void copy_init(unsigned int state, struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);

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
        return copy_from_client(key, data);
    } else if (key->fd == data->origin_fd) {
        return copy_from_origin(key, data);
    } else {
        return ERROR; // Invalid file descriptor
    }
}

unsigned copy_write(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    //Check who is sending data
    if (key->fd == data->client_fd) {
        return copy_from_origin(key, data);
    } else if (key->fd == data->origin_fd) {
        return copy_from_client(key, data);
    } else {
        return ERROR; // Invalid file descriptor
    }
}

static unsigned copy_from_client(struct selector_key *key, struct client_data *data) {
    size_t read_limit;      // Maximum bytes to read in this operation
    ssize_t read_count;      // Total bytes read in this operation

    if (!buffer_can_write(&data->origin_buffer)) {
        return COPY;
    }

    uint8_t *read_buffer = buffer_write_ptr(&data->origin_buffer, &read_limit);
    read_count = recv(key->fd, read_buffer, read_limit, 0);

    if(read_count <= 0) {
        return ERROR;
    }

    buffer_write_adv(&data->origin_buffer, read_count);

    //Ahora origen tiene que enviar los datos al cliente. LO SETEO PARA ESCRIBIR.
    if (selector_set_interest(key->s, data->origin_fd, OP_WRITE) != SELECTOR_SUCCESS) {
        return ERROR;
    }
    return COPY; // Continue copying from client to origin
}

static unsigned copy_from_origin(struct selector_key *key, struct client_data *data) {
    size_t read_limit;      // Maximum bytes to read in this operation
    ssize_t read_count;      // Total bytes read in this operation

    if (!buffer_can_write(&data->client_buffer)) {
        return COPY;
    }

    uint8_t *read_buffer = buffer_write_ptr(&data->client_buffer, &read_limit);
    read_count = recv(key->fd, read_buffer, read_limit, 0);

    if(read_count <= 0) {
        return ERROR;
    }

    buffer_write_adv(&data->client_buffer, read_count);

    //Ahora cliente tiene que enviar los datos al origen. LO SETEO PARA ESCRIBIR.
    if (selector_set_interest(key->s, data->client_fd, OP_WRITE) != SELECTOR_SUCCESS) {
        return ERROR;
    }
    return COPY; // Continue copying from origin to client
}

void copy_close(unsigned int state, struct selector_key *key) {
    printf("closing copy...\n");
}