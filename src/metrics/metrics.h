#ifndef METRICS_H
#define METRICS_H

#include <stdlib.h>

/**
 *  Struct for storing metrics data.
*/
typedef struct metrics_data_t{
    size_t current_connections; // Current number of connections
    size_t total_connections; // Total number of connections since server start
    size_t bytes_sent;
    size_t bytes_received; // Total bytes sent and received
}metrics_data_t;

/**
 * Initializes the metrics data structure.
 */
void metrics_init();

/**
 * Registers new client connection.
 */
void metrics_register_new_connection();

/**
 * Registers client disconnec.
 */
void metrics_register_disconnect();

/**
 * Registers bytes transferred during a connection.
 *
 * @param bytes_sent Number of bytes sent by the server.
 * @param bytes_received Number of bytes received by the server.
 */
void metrics_register_bytes_transferred(size_t bytes_sent, size_t bytes_received);

/**
 * Gets the current metrics data.
 *
 * @return Pointer to the metrics data structure.
 */
void get_metrics_data(metrics_data *data);


#endif //METRICS_H
