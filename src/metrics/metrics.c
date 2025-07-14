#include "metrics.h"

#include <string.h>

static metrics_data_t data;

void metrics_init() {
    memset(&data, 0, sizeof(metrics_data_t));
}

void metrics_register_new_connection() {
    data.current_connections++;
    data.total_connections++;
}

void metrics_register_disconnect() {
    if (data.current_connections > 0) {
        data.current_connections--;
    }
}

void metrics_register_bytes_transferred(size_t bytes_sent, size_t bytes_received) {
    data.bytes_sent += bytes_sent;
    data.bytes_received += bytes_received;
}

void get_metrics_data(metrics_data_t *metrics_data) {
    if (metrics_data != NULL) {
        memcpy(metrics_data, &data, sizeof(metrics_data_t));
    }
}