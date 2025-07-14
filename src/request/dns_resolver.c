#include "dns_resolver.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include "../socks5/socks5.h"
#include "../netutils.h"
#include <netdb.h>


void dns_resolver_init(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    struct request_parser *parser = &data->client.request_parser;


    if (parser->dst_addr_length >= sizeof(data->dns_host)) {
        parser->dst_addr_length = sizeof(data->dns_host) - 1;
    }
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
        return;
    }
    selector_notify_block(data->selector, data->client_fd);
}

void dns_resolution_cancel(struct selector_key *key) {
    struct client_data *data = ATTACHMENT(key);
    if (data->dns_req.ar_result == NULL) return;
}
