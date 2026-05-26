#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <netinet/in.h>

#define SERVER_BACKLOG 10
#define MESSAGE_BUFFER_SIZE 1024
#define COMMAND_LEN 16
#define PAYLOAD_LEN 512

typedef struct {
    char command[COMMAND_LEN];
    int player_id;
    char payload[PAYLOAD_LEN];
} NetworkMessage;

typedef struct {
    int socket_fd;
    int seat;
    int connected;
    struct sockaddr_in address;
} ClientConnection;

int init_server(int port);
int accept_client(int server_fd);
void broadcast_game(int fds[], int num_clients, const char *msg);
int send_message(int socket_fd, const char *msg);
NetworkMessage parse_client_msg(const char *buf);

#endif