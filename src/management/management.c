#include "management.h"
#include "management_auth.h"
#include "management_command.h"
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>

static void nothing() {
    // This function does nothing and is used as a placeholder
}

static const struct state_definition management_states[] = {
        {
            .state = MANAGEMENT_AUTH_READ,
            .on_arrival = management_auth_init,
            .on_read_ready = management_auth_read,
        },
        {
            .state = MANAGEMENT_AUTH_WRITE,
            .on_write_ready = management_auth_write,
        },
        {
            .state = MANAGEMENT_REQUEST_READ,
            .on_arrival = management_command_read_init,
            .on_read_ready = management_command_read,
        },
        {
            .state = MANAGEMENT_REQUEST_WRITE,
            .on_arrival = nothing,
            .on_write_ready = management_command_write,
        },
        {
            .state = MANAGEMENT_CLOSED,
            .on_arrival = nothing,
        },
        {
            .state = MANAGEMENT_ERROR,
            .on_arrival = nothing,
        }
};

static void management_v1_close_connection(struct selector_key *key);
static void management_v1_read(struct selector_key *key);
static void management_v1_write(struct selector_key *key);
static void management_v1_close(struct selector_key *key);
static void management_v1_block(struct selector_key *key);

static struct fd_handler management_v1_handler = {
    .handle_read = management_v1_read,
    .handle_write = management_v1_write,
    .handle_block = management_v1_block,
    .handle_close = management_v1_close,
};

static void management_v1_close(struct selector_key *key) {
    struct state_machine *stm = &ATTACHMENT_MANAGEMENT(key)->sm;
    stm_handler_close(stm, key);
    management_v1_close_connection(key);
}

static void management_v1_read(struct selector_key *key) {
    struct state_machine *stm = &ATTACHMENT_MANAGEMENT(key)->sm;
    management_state state = stm_handler_read(stm, key);
    if(state == MANAGEMENT_CLOSED || state == MANAGEMENT_ERROR) {
        management_v1_close_connection(key);
    }
}

static void management_v1_write(struct selector_key *key) {
    struct state_machine *stm = &ATTACHMENT_MANAGEMENT(key)->sm;
    management_state state = stm_handler_write(stm, key);
    if(state == MANAGEMENT_CLOSED || state == MANAGEMENT_ERROR) {
        management_v1_close_connection(key);
    }
}

static void management_v1_block(struct selector_key *key) {
    struct state_machine *stm = &ATTACHMENT_MANAGEMENT(key)->sm;
    management_state state = stm_handler_block(stm, key);
    if(state == MANAGEMENT_CLOSED || state == MANAGEMENT_ERROR) {
        management_v1_close_connection(key);
    }
}

void management_v1_passive_accept(struct selector_key *key) {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    int client_fd = accept(key->fd, (struct sockaddr *) &addr, &addr_len);
    if (client_fd < 0) {
        perror("accept");
        return;
    }
    if (client_fd > FD_SETSIZE) {
        fprintf(stderr, "Client fd exceeds FD_SETSIZE\n");
        close(client_fd);
        return;
    }

    management_client *client = calloc(1, sizeof(management_client));
    if (client == NULL) {
        perror("calloc");
        close(client_fd);
        return;
    }

    printf("New management client connected: fd=%d\n", client_fd);
    client->sm.initial = MANAGEMENT_AUTH_READ;
    client->sm.max_state = MANAGEMENT_ERROR;
    client->closed = false;
    client->sm.states = management_states;
    client->client_fd = client_fd;
    client->is_admin = false; // Default to non-admin user

    buffer_init(&client->request_buffer, REQUEST_BUFFER_SIZE, client->request_buffer_data);
    buffer_init(&client->response_buffer, RESPONSE_BUFFER_SIZE, client->response_buffer_data);
    stm_init(&client->sm);

    selector_status status = selector_register(key->s, client_fd, &management_v1_handler, OP_READ, client);
    if (status != SELECTOR_SUCCESS) {
        perror("selector_register");
        free(client);
        close(client_fd);
        return;
    }
}

static void management_v1_close_connection(struct selector_key *key) {
    management_client *client = ATTACHMENT_MANAGEMENT(key);
    if (client->closed) {
        return; // Already closed
    }
    client->closed = true;
    printf("Closing management connection: fd=%d\n", client->client_fd);

    if (client->client_fd >= 0) {
        selector_unregister_fd(key->s, client->client_fd);
        close(client->client_fd);
        client->client_fd = -1;
    }

    free(client);
}
