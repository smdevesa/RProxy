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
            fprintf(stderr, "Dirección IPv6 inválida: %s\n", addr);
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
        fprintf(stderr, "Dirección IPv4 inválida: %s\n", addr);
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
    printf("BIENVENIDOS AL SERVIDOR RPROXY\n");

    users_init();
    printf("Usuario por defecto creado: %s\n", DEFAULT_ADMIN_USERNAME);

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
        fprintf(stderr, "Error al inicializar el selector\n");
        exit(1);
    }
    struct fdselector *selector = selector_new(FD_SETSIZE);
    if(selector == NULL) {
        fprintf(stderr, "Error al crear el selector\n");
        selector_close();
        exit(1);
    }

    struct socks5args args;
    parse_args(argc, argv, &args);
    struct sockaddr_storage aux;
    memset(&aux, 0, sizeof(aux));
    socklen_t aux_len = sizeof(aux);
    int server = -1;

    if(setup_sock_addr(args.socks_addr, args.socks_port, &aux, &aux_len) < 0) {
        fprintf(stderr, "Error al configurar la dirección del servidor SOCKS\n");
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
        fprintf(stderr, "Error al registrar el socket del servidor SOCKS: %s\n", selector_error(ss));
        goto finally;
    }

    printf("Servidor SOCKS5 escuchando en %s:%d\n", args.socks_addr, args.socks_port);
    while(true) {
        ss = selector_select(selector);
        if(ss != SELECTOR_SUCCESS) {
            fprintf(stderr, "Error en selector_select: %s\n", selector_error(ss));
            goto finally;
        }
    }

    finally:
    if(server >= 0) {
        close(server);
    }
    if(selector != NULL) {
        selector_destroy(selector);
    }
    selector_close();
    printf("Servidor SOCKS5 cerrado.\n");
    return 0;
}
