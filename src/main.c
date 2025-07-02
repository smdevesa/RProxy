// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 12345
#define MAX_CLIENTS  FD_SETSIZE
#define BUFFER_SIZE 1024

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int listen_fd, new_fd, max_fd, i;
    int client_fds[MAX_CLIENTS];
    fd_set read_fds, all_fds;
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE];
    
    // Inicializar array clientes
    for(i=0; i<MAX_CLIENTS; i++) client_fds[i] = -1;

    // Crear socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Permitir reutilizar puerto
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if(bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // Escuchar
    if(listen(listen_fd, SOMAXCONN) < 0) {
        perror("listen");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // Set non-blocking
    if(set_nonblocking(listen_fd) < 0) {
        perror("set_nonblocking");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&all_fds);
    FD_SET(listen_fd, &all_fds);
    max_fd = listen_fd;

    printf("Echo server no bloqueante escuchando en puerto %d\n", PORT);

    while(1) {
        read_fds = all_fds;
        int n_ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if(n_ready < 0) {
            if(errno == EINTR) continue;
            perror("select");
            break;
        }

        // Nuevo cliente
        if(FD_ISSET(listen_fd, &read_fds)) {
            struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);
            new_fd = accept(listen_fd, (struct sockaddr*)&cli_addr, &cli_len);
            if(new_fd < 0) {
                if(errno != EWOULDBLOCK && errno != EAGAIN) perror("accept");
            } else {
                if(set_nonblocking(new_fd) < 0) {
                    perror("set_nonblocking new_fd");
                    close(new_fd);
                } else {
                    // Añadir a lista clientes
                    for(i=0; i<MAX_CLIENTS; i++) {
                        if(client_fds[i] == -1) {
                            client_fds[i] = new_fd;
                            FD_SET(new_fd, &all_fds);
                            if(new_fd > max_fd) max_fd = new_fd;
                            printf("Cliente conectado: fd=%d\n", new_fd);
                            break;
                        }
                    }
                    if(i == MAX_CLIENTS) {
                        printf("Demasiados clientes\n");
                        close(new_fd);
                    }
                }
            }
            if(--n_ready <= 0) continue;
        }

        // Revisar clientes
        for(i=0; i<MAX_CLIENTS; i++) {
            int fd = client_fds[i];
            if(fd == -1) continue;
            if(FD_ISSET(fd, &read_fds)) {
                ssize_t n = read(fd, buffer, sizeof(buffer));
                if(n <= 0) {
                    if(n == 0) {
                        printf("Cliente desconectado: fd=%d\n", fd);
                    } else if(errno != EWOULDBLOCK && errno != EAGAIN) {
                        perror("read");
                    }
                    close(fd);
                    FD_CLR(fd, &all_fds);
                    client_fds[i] = -1;
                } else {
                    // Echo: escribir lo que leyó
                    ssize_t total_written = 0;
                    while(total_written < n) {
                        ssize_t w = write(fd, buffer + total_written, n - total_written);
                        if(w <= 0) {
                            if(errno == EWOULDBLOCK || errno == EAGAIN) {
                                // No puede escribir ahora, salimos para evitar busy wait
                                break;
                            } else {
                                perror("write");
                                close(fd);
                                FD_CLR(fd, &all_fds);
                                client_fds[i] = -1;
                                break;
                            }
                        }
                        total_written += w;
                    }
                }
                if(--n_ready <= 0) break;
            }
        }
    }

    close(listen_fd);
    return 0;
}
