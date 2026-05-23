#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "socket_server.h"

int init_server(int port)
{
    int server_fd;
    int opt = 1;
    struct sockaddr_in address;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket failed");
        return -1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((unsigned short)port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, SERVER_BACKLOG) < 0) {
        perror("listen failed");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

int accept_client(int server_fd)
{
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);

    int client_fd = accept(
        server_fd,
        (struct sockaddr *)&client_address,
        &client_len
    );

    if (client_fd < 0) {
        perror("accept failed");
        return -1;
    }

    printf("New client connected from %s:%d\n",
           inet_ntoa(client_address.sin_addr),
           ntohs(client_address.sin_port));

    return client_fd;
}

int send_message(int socket_fd, const char *msg)
{
    if (socket_fd < 0 || msg == NULL) {
        return -1;
    }

    ssize_t sent = send(socket_fd, msg, strlen(msg), 0);

    if (sent < 0) {
        perror("send failed");
        return -1;
    }

    return (int)sent;
}

void broadcast_game(int fds[], int num_clients, const char *msg)
{
    if (fds == NULL || msg == NULL) {
        return;
    }

    for (int i = 0; i < num_clients; i++) {
        if (fds[i] >= 0) {
            send_message(fds[i], msg);
        }
    }
}

NetworkMessage parse_client_msg(const char *buf)
{
    NetworkMessage message;

    memset(&message, 0, sizeof(message));
    snprintf(message.command, COMMAND_LEN, "UNKNOWN");
    message.player_id = -1;
    message.payload[0] = '\0';

    if (buf == NULL) {
        return message;
    }

    /*
     * Expected format:
     * COMMAND:PLAYER_ID:PAYLOAD
     *
     * Examples:
     * LOGIN:-1:Hamza
     * SEAT:-1:2
     * ACTN:0:CHECK
     * RAISE:0:50
     */
    char copy[MESSAGE_BUFFER_SIZE];
    snprintf(copy, sizeof(copy), "%s", buf);

    char *command = strtok(copy, ":");
    char *player_id = strtok(NULL, ":");
    char *payload = strtok(NULL, "\n");

    if (command != NULL) {
        snprintf(message.command, COMMAND_LEN, "%s", command);
    }

    if (player_id != NULL) {
        message.player_id = atoi(player_id);
    }

    if (payload != NULL) {
        snprintf(message.payload, PAYLOAD_LEN, "%s", payload);
    }

    return message;
}