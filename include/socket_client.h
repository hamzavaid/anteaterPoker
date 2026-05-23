#ifndef SOCKET_CLIENT_H
#define SOCKET_CLIENT_H

/*
 * socket_client.h
 *
 * This file defines the client-side socket interface.
 * These functions allow the poker client to connect to the server,
 * send messages, and receive messages.
 */

/* Maximum size for messages sent between client and server. */
#define CLIENT_BUFFER_SIZE 1024

/*
 * Connects the client to the server.
 *
 * Parameters:
 * - host: server hostname or IP address, such as "localhost"
 * - port: server port number, such as 10010
 *
 * Returns:
 * - socket file descriptor if successful
 * - -1 if connection fails
 */
int connect_to_server(const char *host, int port);

/*
 * Sends a text message to the server.
 *
 * Parameters:
 * - socket_fd: connected socket file descriptor
 * - message: message string to send
 *
 * Returns:
 * - number of bytes sent if successful
 * - -1 if sending fails
 */
int send_to_server(int socket_fd, const char *message);

/*
 * Receives a text message from the server.
 *
 * Parameters:
 * - socket_fd: connected socket file descriptor
 * - buffer: output buffer to store received message
 * - buffer_size: size of the output buffer
 *
 * Returns:
 * - number of bytes received
 * - 0 if server disconnected
 * - -1 if receiving fails
 */
int receive_from_server(int socket_fd, char *buffer, int buffer_size);

#endif