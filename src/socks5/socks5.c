#include "socks5.h"
#include <stdio.h>
#include <stdlib.h>

#include "../copy.h"
#include "../stm.h"
#include "../handshake/handshake.h"
#include "../auth/auth.h"
#include "../request/request.h"

// Forward declarations of state handlers
static void socks_v5_read(struct selector_key *key);
static void socks_v5_write(struct selector_key *key);
static void socks_v5_block(struct selector_key *key);
static void socks_v5_close(struct selector_key *key);
static void handle_error(const unsigned state,struct selector_key *key);
static void handle_done(const unsigned state, struct selector_key *key);

static fd_handler socks_v5_handler = {
    .handle_read = socks_v5_read,
    .handle_write = socks_v5_write,
    .handle_block = socks_v5_block,
    .handle_close = socks_v5_close,
};

void nothing(const unsigned int s, struct selector_key *key) {
    // This function does nothing, it is used as a placeholder for states that do not require any action.
}



static const struct state_definition socks_v5_states[] = {
    {
            .state = HANDSHAKE_READ,
            .on_arrival = handshake_read_init,
            .on_read_ready = handshake_read,
        },
    {
            .state = HANDSHAKE_WRITE,
            .on_write_ready = handshake_write,
        },
    {
            .state = AUTH_READ,
            .on_arrival = auth_read_init,
            .on_read_ready = auth_read,
        },
    {
            .state = AUTH_WRITE,
            .on_write_ready = auth_write,
        },
    {
            .state = REQUEST_READ,
            .on_arrival = request_read_init,
            .on_read_ready = request_read,
        },
    {
            .state = REQUEST_DNS,
            .on_block_ready = request_DNS_completed,
        },
    {
            .state = REQUEST_CONNECT,
            .on_write_ready = request_connect,
        },
    {
            .state = REQUEST_WRITE,
            .on_write_ready = request_write,
        },
    {
            .state = COPY,
            .on_arrival = copy_init,
            .on_read_ready = copy_read,
            .on_write_ready = copy_write,
            .on_departure = copy_close,
        },
    {
            .state = DONE,
            .on_arrival = handle_done,
        },
    {
            .state = ERROR,
            .on_arrival = handle_error,
        }
};

void socks_v5_passive_accept(struct selector_key *key) {
    struct sockaddr_storage client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int new_client_fd = accept(key->fd, (struct sockaddr *)&client_addr, &addr_len);
    if (new_client_fd < 0) {
        perror("accept");
        return;
    }
    if(new_client_fd > FD_SETSIZE) {
        fprintf(stderr, "New client fd %d exceeds FD_SETSIZE %d\n", new_client_fd, FD_SETSIZE);
        close(new_client_fd);
        return;
    }
    struct client_data *data = calloc(1, sizeof(struct client_data));
    if (data == NULL) {
        perror("calloc");
        close(new_client_fd);
        return;
    }

    printf("New client connected: fd=%d\n", new_client_fd);
    data->sm.initial = HANDSHAKE_READ;
    data->sm.max_state = ERROR;
    data->closed = false;
    data->sm.states = socks_v5_states;
    data->client_fd = new_client_fd;
    data->origin_fd = -1; // Initially, no origin connection
    data->client_addr = client_addr;

    buffer_init(&data->client_buffer, BUFFER_SIZE, data->client_buffer_data);
    buffer_init(&data->origin_buffer, BUFFER_SIZE, data->origin_buffer_data);
    stm_init(&data->sm);

    selector_status status = selector_register(key->s, new_client_fd, &socks_v5_handler, OP_READ, data);
    if (status != SELECTOR_SUCCESS) {
        perror("selector_register");
        free(data);
        close(new_client_fd);
        return;
    }
}

void close_connection(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    if (data->closed) {
        return; // Already closed
    }
    data->closed = true;
    printf("Closing connection: fd=%d\n", data->client_fd);

    if (data->client_fd >= 0) {
        selector_unregister_fd(key->s, data->client_fd);
        close(data->client_fd);
        data->client_fd = -1;
    }
    if (data->origin_fd >= 0) {
        selector_unregister_fd(key->s, data->origin_fd);
        close(data->origin_fd);
        data->origin_fd = -1;
    }
    free(data);
}

selector_status register_origin_selector(struct selector_key *key, int origin_fd, struct client_data *data){
    return selector_register(key->s, origin_fd, &socks_v5_handler, OP_READ, data);
}


static void socks_v5_read(struct selector_key *key) {
    struct state_machine *sm = &ATTACHMENT(key)->sm;
    enum socks5_state state = stm_handler_read(sm, key);
    if (state == ERROR || state == DONE) {
        close_connection(key);
        return;
    }
}

static void socks_v5_write(struct selector_key *key) {
    struct state_machine *sm = &ATTACHMENT(key)->sm;
    enum socks5_state state = stm_handler_write(sm, key);
    if (state == ERROR || state == DONE) {
        close_connection(key);
        return;
    }
}

static void socks_v5_block(struct selector_key *key) {
    struct state_machine *sm = &ATTACHMENT(key)->sm;
    enum socks5_state state = stm_handler_block(sm, key);
    if (state == ERROR || state == DONE) {
        close_connection(key);
        return;
    }
}

static void socks_v5_close(struct selector_key *key) {
    struct state_machine *sm = &ATTACHMENT(key)->sm;
    stm_handler_close(sm, key);
    close_connection(key);
}

static void handle_error(const unsigned state, struct selector_key *key) {
    printf("Error in state %u, closing connection\n", state);
}

static void handle_done(const unsigned state, struct selector_key *key) {
    printf("Done in state %u, closing connection\n", state);
}