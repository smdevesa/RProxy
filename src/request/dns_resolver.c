#include "dns_resolver.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include "../socks5/socks5.h"
#include "../netutils.h"
#include <netdb.h>


static void cleanup_previous_resolution(struct client_data *d) {
    if (d->origin_addrinfo != NULL) {
        if (d->resolution_from_getaddrinfo)
            freeaddrinfo(d->origin_addrinfo);
        else
            free(d->origin_addrinfo);
        d->origin_addrinfo = NULL;
    }
}

static void create_direct_addrinfo(struct client_data *data, int family, const void *raw_addr,uint16_t port) {
    cleanup_previous_resolution(data);
    data->origin_addrinfo = calloc(1, sizeof(struct addrinfo));
    if (!data->origin_addrinfo) return;

    if (family == AF_INET) {
        struct sockaddr_in *sa = calloc(1, sizeof *sa);
        memcpy(&sa->sin_addr, raw_addr, 4);
        sa->sin_family = AF_INET;
        sa->sin_port   = htons(port);

        data->origin_addrinfo->ai_family = AF_INET;
        data->origin_addrinfo->ai_socktype = SOCK_STREAM;
        data->origin_addrinfo->ai_addr = (struct sockaddr *) sa;
        data->origin_addrinfo->ai_addrlen = sizeof *sa;

    } else { /* AF_INET6 */
        struct sockaddr_in6 *sa6 = calloc(1, sizeof *sa6);
        memcpy(&sa6->sin6_addr, raw_addr, 16);
        sa6->sin6_family = AF_INET6;
        sa6->sin6_port = htons(port);

        data->origin_addrinfo->ai_family = AF_INET6;
        data->origin_addrinfo->ai_socktype = SOCK_STREAM;
        data->origin_addrinfo->ai_addr = (struct sockaddr *) sa6;
        data->origin_addrinfo->ai_addrlen = sizeof *sa6;
    }
    data->resolution_from_getaddrinfo = false;
}

void dns_resolver_init(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    struct request_parser *parser = &data->client.request_parser;


    printf(">> dns_resolver_init host='%.*s' port=%zu\n",(int)parser->dst_addr_length, parser->dst_addr, parser->dst_port);

    if (parser->dst_addr_length >= sizeof(data->dns_host)) {
        parser->dst_addr_length = sizeof(data->dns_host) - 1;
    }
    printf(">>> About to copy domain: len=%zu, addr=%p\n",
       parser->dst_addr_length, (void *)parser->dst_addr);
    memcpy(data->dns_host, parser->dst_addr, parser->dst_addr_length);
    data->dns_host[parser->dst_addr_length] = '\0';

    snprintf(data->dns_port, sizeof(data->dns_port), "%hu", (unsigned short)parser->dst_port);

    memset(&data->dns_req, 0, sizeof data->dns_req);
    data->dns_req.ar_name = data->dns_host;
    data->dns_req.ar_service = data->dns_port;

    static struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_socktype = SOCK_STREAM;
    data->dns_req.ar_request = &hints;

    printf(">>> dns_resolver_init: key->s = %p\n", (void*)key->s);

    data->selector = key->s;

    struct sigevent sev = {
        .sigev_notify          = SIGEV_THREAD,
        .sigev_notify_function = dns_resolution_done,
        .sigev_value.sival_ptr = data,
        .sigev_notify_attributes = NULL
    };

    struct gaicb *reqs[1] = { &data->dns_req };
    data->dns_req.ar_result = NULL;
    if (getaddrinfo_a(GAI_NOWAIT, reqs, 1, &sev) != 0) {
        printf(">> dns_resolver_init lanzó getaddrinfo_a OK\n");
        request_build_response(parser, &data->origin_buffer,
                               REQUEST_REPLY_HOST_UNREACHABLE);
        selector_set_interest_key(key, OP_WRITE);
        return;
    }

    selector_set_interest_key(key, OP_NOOP);
}

void dns_resolution_done(union sigval sv) {
    struct client_data *data = sv.sival_ptr;
    if (data == NULL || data->selector == NULL) {
        fprintf(stderr, ">>> ERROR: datos nulos\n");
        return;
    }
    printf(">>> DNS resolution completed for host=%s\n", data->dns_host);
    printf("selector_notify_block(data->selector, %d)\n", data->client_fd);
    selector_notify_block(data->selector, data->client_fd);  // o -1 si usás otra lógica
}

void dns_resolution_cancel(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    if (data->dns_req.ar_result == NULL) return;
}
