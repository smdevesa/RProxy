#include "copy.h"
#include "errno.h"
#include "metrics/metrics.h"

static unsigned read_from_client(struct selector_key *key, struct client_data *data);
static unsigned read_from_origin(struct selector_key *key, struct client_data *data);
static unsigned write_from_client(struct selector_key *key, struct client_data *data);
static unsigned write_from_origin(struct selector_key *key, struct client_data *data);

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
        return read_from_client(key, data);
    } else if (key->fd == data->origin_fd) {
        return read_from_origin(key, data);
    } else {
        return ERROR; // Invalid file descriptor
    }
}

unsigned copy_write(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);

    if (key->fd == data->client_fd) {
        return write_from_origin(key, data);
    } else if (key->fd == data->origin_fd) {
        return write_from_client(key, data);
    } else {
        return ERROR;
    }
}


static unsigned read_from_client(struct selector_key *key, struct client_data *data) {
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
        return DONE;  // o un estado específico para manejar el cierre
    }

    buffer_write_adv(&data->origin_buffer, read_count);

    // Intentar escribir inmediatamente al origen
    size_t write_limit;
    uint8_t *write_buffer = buffer_read_ptr(&data->origin_buffer, &write_limit);
    ssize_t write_count = send(data->origin_fd, write_buffer, write_limit, MSG_NOSIGNAL);

    if (write_count > 0) {
        buffer_read_adv(&data->origin_buffer, write_count);
    }

    if (buffer_can_read(&data->origin_buffer) || (write_count < 0 && errno == EWOULDBLOCK)) {
        if (selector_set_interest(key->s, data->origin_fd, OP_WRITE) != SELECTOR_SUCCESS) {
            return ERROR;
        }
    }
    return COPY;
}

static unsigned read_from_origin(struct selector_key *key, struct client_data *data) {
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
        return DONE;
    }

    buffer_write_adv(&data->client_buffer, read_count);

    size_t write_limit;
    uint8_t *write_buffer = buffer_read_ptr(&data->client_buffer, &write_limit);
    ssize_t write_count = send(data->client_fd, write_buffer, write_limit, MSG_NOSIGNAL);

    if (write_count > 0) {
        buffer_read_adv(&data->client_buffer, write_count);
        metrics_register_bytes_transferred(0, write_count);
    }

    if (buffer_can_read(&data->client_buffer) || (write_count < 0 && errno == EWOULDBLOCK)) {
        if (selector_set_interest(key->s, data->client_fd, OP_WRITE) != SELECTOR_SUCCESS) {
            return ERROR;
        }
    }
    return COPY;
}

static unsigned write_from_client(struct selector_key *key, struct client_data *data){
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

    metrics_register_bytes_transferred(write_count, 0);

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
    size_t write_limit;
    ssize_t write_count;

    if (!buffer_can_read(&data->client_buffer)){
        return COPY;  // No hay datos para escribir
    }

    uint8_t *write_buffer = buffer_read_ptr(&data->client_buffer, &write_limit);
    write_count = send(key->fd, write_buffer, write_limit, MSG_NOSIGNAL);

    if (write_count <= 0) {
        perror("send");
        return ERROR;
    }

    buffer_read_adv(&data->client_buffer, write_count);

    if (!buffer_can_read(&data->client_buffer)){
        // Si no hay más datos para escribir, volver a modo lectura
        if (selector_set_interest(key->s, data->client_fd, OP_READ) != SELECTOR_SUCCESS){
            return ERROR;
        }
    }

    return COPY;
}

void copy_close(unsigned int state, struct selector_key *key) {
}