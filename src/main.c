// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "socks5/socks5.h"
#include "selector.h"
#include "args.h"
#include <sys/select.h>
#include "users.h"
#include "management/management.h"
#include "metrics/metrics.h"
#include "config.h"


#define MAX_CLIENTS  FD_SETSIZE
#define MAX_PENDING 20 // Número máximo de conexiones pendientes

static int setup_sock_addr(char *addr, unsigned short port, void *result, socklen_t *result_len) {
    int ipv6 = strchr(addr, ':') != NULL;

    if(ipv6) {
        struct sockaddr_in6 sock_ipv6;
        memset(&sock_ipv6, 0, sizeof(sock_ipv6));
        sock_ipv6.sin6_family = AF_INET6;
        sock_ipv6.sin6_port = htons(port);
        if(inet_pton(AF_INET6, addr, &sock_ipv6.sin6_addr) <= 0) {
            return -1;
        }
        *(struct sockaddr_in6 *)result = sock_ipv6;
        *result_len = sizeof(sock_ipv6);
        return 0;
    }

    struct sockaddr_in sock_ipv4;
    memset(&sock_ipv4, 0, sizeof(sock_ipv4));
    sock_ipv4.sin_family = AF_INET;
    sock_ipv4.sin_port = htons(port);
    if(inet_pton(AF_INET, addr, &sock_ipv4.sin_addr) <= 0) {
        return -1;
    }
    *(struct sockaddr_in *)result = sock_ipv4;
    *result_len = sizeof(sock_ipv4);
    return 0;
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0); // Desactivar buffering de stdout
    setvbuf(stderr, NULL, _IONBF, 0); // Desactivar buffering de stderr
    close(STDIN_FILENO);                    // Cerrar stdin para evitar bloqueos
    signal(SIGPIPE, SIG_IGN);               // Ignorar señales SIGPIPE para evitar cierres inesperados
    int mng_server = -1;


    printf("RProxy SOCKS5 Server 1.0\n");
    printf("Default admin user credentials: admin:1234\n");

    if(!config_init()) {
        fprintf(stderr, "Error initializing configuration module. Aborting.\n");
        exit(1);
    }

    // Creacion del selector
    selector_status ss = SELECTOR_SUCCESS;
    struct selector_init config = {
            .signal = SIGALRM,
            .select_timeout = {
                    .tv_sec = 10,
                    .tv_nsec = 0
            },
    };
    if(selector_init(&config) != 0) {
        fprintf(stderr, "Error initializing selector. Aborting.\n");
        exit(1);
    }
    struct fdselector *selector = selector_new(FD_SETSIZE);
    if(selector == NULL) {
        fprintf(stderr, "Error creating selector. Aborting.\n");
        selector_close();
        exit(1);
    }
    metrics_init();
    users_init();

    struct socks5args args;
    parse_args(argc, argv, &args);

    for (int i = 0; i < MAX_USERS_ARGS; i++) {
        if (args.users[i].name != NULL && args.users[i].pass != NULL) {
            // Añadir el usuario al sistema
            if (!create_user(args.users[i].name, args.users[i].pass, false)) {
                fprintf(stderr, "Error al añadir usuario %s\n", args.users[i].name);
            } else {
                printf("Usuario añadido: %s\n", args.users[i].name);
            }
        } else {
            // No hay más usuarios
            break;
        }
    }

    struct sockaddr_storage aux;
    memset(&aux, 0, sizeof(aux));
    socklen_t aux_len = sizeof(aux);
    int server = -1;

    if(setup_sock_addr(args.socks_addr, args.socks_port, &aux, &aux_len) < 0) {
        fprintf(stderr, "Error setting up socket address for %s:%d Aborting.", args.socks_addr, args.socks_port);
        selector_close();
        exit(1);
    }

    server = socket(aux.ss_family, SOCK_STREAM, IPPROTO_TCP);
    if(server < 0) {
        perror("socket");
        selector_close();
        exit(1);
    }

    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if(bind(server, (struct sockaddr *)&aux, aux_len) < 0) {
        perror("bind");
        goto finally;
    }
    if(listen(server, MAX_PENDING) < 0) {
        perror("listen");
        goto finally;
    }
    if(selector_fd_set_nio(server) == -1) {
        perror("selector_fd_set_nio");
        goto finally;
    }

    const fd_handler socks_v5 = {
            .handle_read = socks_v5_passive_accept
    };
    ss = selector_register(selector, server, &socks_v5, OP_READ, NULL);

    if(ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "Error registering SOCKS5 server socket: %s\n", selector_error(ss));
        goto finally;
    }

    printf("Socks5 server listening on %s:%d\n", args.socks_addr, args.socks_port);

    struct sockaddr_storage mng_sock_addr;
    socklen_t mng_sock_len = sizeof(mng_sock_addr);
    memset(&mng_sock_addr, 0, sizeof(mng_sock_addr));

    if (setup_sock_addr(args.mng_addr, args.mng_port, &mng_sock_addr, &mng_sock_len) < 0) {
        fprintf(stderr, "Error setting up management socket address for %s:%d Aborting.\n", args.mng_addr, args.mng_port);
        goto finally;
    }

    mng_server = socket(mng_sock_addr.ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (mng_server < 0) {
        perror("socket management");
        goto finally;
    }

    setsockopt(mng_server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (bind(mng_server, (struct sockaddr *)&mng_sock_addr, mng_sock_len) < 0) {
        perror("bind management");
        goto finally;
    }

    if (listen(mng_server, MAX_PENDING) < 0) {
        perror("listen management");
        goto finally;
    }

    if (selector_fd_set_nio(mng_server) < 0) {
        perror("selector_fd_set_nio management");
        goto finally;
    }

    const fd_handler mng_handler = {
        .handle_read = management_v1_passive_accept,
    };

    ss = selector_register(selector, mng_server, &mng_handler, OP_READ, NULL);
    if (ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "Error registering management server socket: %s\n", selector_error(ss));
        goto finally;
    }

    printf("Management server listening on %s:%d\n", args.mng_addr, args.mng_port);
    while(true) {
        ss = selector_select(selector);
        if(ss != SELECTOR_SUCCESS) {
            fprintf(stderr, "Error in selector_select: %s\n", selector_error(ss));
            goto finally;
        }
    }

    finally:
    if(server >= 0) {
        close(server);
    }
    if (mng_server >= 0) {
        close(mng_server);
    }
    if(selector != NULL) {
        selector_destroy(selector);
    }
    config_cleanup();
    selector_close();
    printf("Server shutting down.\n");
    return 0;
}
