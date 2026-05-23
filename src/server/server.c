#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/select.h>

#include "game_state.h"
#include "socket_server.h"

static void parse_server_args(int argc, char *argv[], ServerConfig *config)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            config->port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--table") == 0 && i + 1 < argc) {
            snprintf(config->table_name, MAX_TABLE_NAME_LEN, "%s", argv[++i]);
        } else if (strcmp(argv[i], "--points") == 0 && i + 1 < argc) {
            config->starting_points = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--bots") == 0 && i + 1 < argc) {
            config->bot_count = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
            snprintf(config->log_path, MAX_LOG_PATH_LEN, "%s", argv[++i]);
        }
    }
}

static void collect_client_fds(const GameState *game, int fds[], int *num_clients)
{
    *num_clients = 0;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].status != PLAYER_EMPTY &&
            game->players[i].socket_fd >= 0) {
            fds[*num_clients] = game->players[i].socket_fd;
            (*num_clients)++;
        }
    }
}

static int find_seat_by_socket(const GameState *game, int socket_fd)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].socket_fd == socket_fd &&
            game->players[i].status != PLAYER_EMPTY) {
            return i;
        }
    }

    return -1;
}

static void send_public_state_to_all(const GameState *game)
{
    int fds[MAX_PLAYERS];
    int num_clients = 0;
    char state_msg[MESSAGE_BUFFER_SIZE];

    collect_client_fds(game, fds, &num_clients);
    build_public_game_state(game, state_msg, sizeof(state_msg));
    broadcast_game(fds, num_clients, state_msg);
}

static void send_private_hands(GameState *game)
{
    char hand_msg[MESSAGE_BUFFER_SIZE];

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].status == PLAYER_ACTIVE &&
            game->players[i].socket_fd >= 0) {
            build_private_hand_message(game, i, hand_msg, sizeof(hand_msg));
            send_message(game->players[i].socket_fd, hand_msg);
        }
    }
}

static void handle_client_message(GameState *game, int client_fd, const char *buf)
{
    NetworkMessage msg = parse_client_msg(buf);

    printf("Received: command=%s player=%d payload=%s\n",
           msg.command,
           msg.player_id,
           msg.payload);

    if (strcmp(msg.command, "LOGIN") == 0) {
        int seat = add_player(game, client_fd, msg.payload);

        if (seat < 0) {
            send_message(client_fd, "ERROR:-1:Seat unavailable\n");
            return;
        }

        char reply[MESSAGE_BUFFER_SIZE];
        snprintf(reply, sizeof(reply), "SEAT:%d:Welcome %s\n", seat, msg.payload);
        send_message(client_fd, reply);
        send_public_state_to_all(game);
        return;
    }

    if (strcmp(msg.command, "START") == 0) {
        start_new_hand(game);
        send_public_state_to_all(game);
        send_private_hands(game);
        return;
    }

    if (strcmp(msg.command, "ACTN") == 0) {
        int seat = find_seat_by_socket(game, client_fd);

        if (seat < 0) {
            send_message(client_fd, "ERROR:-1:Player not logged in\n");
            return;
        }

        if (seat != game->current_turn) {
            send_message(client_fd, "ERROR:-1:Not your turn\n");
            return;
        }

        if (strcmp(msg.payload, "FOLD") == 0) {
            game->players[seat].status = PLAYER_FOLDED;
            send_message(client_fd, "OK:-1:Fold accepted\n");
        } else if (strcmp(msg.payload, "CHECK") == 0) {
            send_message(client_fd, "OK:-1:Check accepted\n");
        } else if (strcmp(msg.payload, "CALL") == 0) {
            send_message(client_fd, "OK:-1:Call accepted\n");
        } else {
            send_message(client_fd, "ERROR:-1:Illegal action\n");
            return;
        }

        /* Simple turn advancement for now. Later, rules module should handle this. */
        for (int i = 1; i <= MAX_PLAYERS; i++) {
            int next = (seat + i) % MAX_PLAYERS;

            if (game->players[next].status == PLAYER_ACTIVE) {
                game->current_turn = next;
                break;
            }
        }

        send_public_state_to_all(game);
        return;
    }

    if (strcmp(msg.command, "RAISE") == 0) {
        int seat = find_seat_by_socket(game, client_fd);
        int amount = atoi(msg.payload);

        if (seat < 0) {
            send_message(client_fd, "ERROR:-1:Player not logged in\n");
            return;
        }

        if (amount <= 0 || amount > game->players[seat].points) {
            send_message(client_fd, "ERROR:-1:Invalid raise amount\n");
            return;
        }

        game->players[seat].points -= amount;
        game->players[seat].current_bet += amount;
        game->pot += amount;
        game->current_bet = game->players[seat].current_bet;

        send_message(client_fd, "OK:-1:Raise accepted\n");
        send_public_state_to_all(game);
        return;
    }

    if (strcmp(msg.command, "QUIT") == 0) {
        int seat = find_seat_by_socket(game, client_fd);

        if (seat >= 0) {
            remove_player(game, seat);
        }

        close(client_fd);
        send_public_state_to_all(game);
        return;
    }

    send_message(client_fd, "ERROR:-1:Unknown command\n");
}

int main(int argc, char *argv[])
{
    ServerConfig config;
    GameState game;

    init_server_config(&config);
    parse_server_args(argc, argv, &config);

    int server_fd = init_server(config.port);

    if (server_fd < 0) {
        fprintf(stderr, "Failed to start server.\n");
        return 1;
    }

    init_game_state(&game, &config, server_fd);

    printf("Anteater Poker server started.\n");
    printf("Table: %s\n", config.table_name);
    printf("Port: %d\n", config.port);
    printf("Waiting for clients...\n");

    fd_set read_fds;
    int max_fd;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;

        for (int i = 0; i < MAX_PLAYERS; i++) {
            int fd = game.players[i].socket_fd;

            if (game.players[i].status != PLAYER_EMPTY && fd >= 0) {
                FD_SET(fd, &read_fds);

                if (fd > max_fd) {
                    max_fd = fd;
                }
            }
        }

        int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (ready < 0) {
            perror("select failed");
            break;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            int client_fd = accept_client(server_fd);

            if (client_fd >= 0) {
                send_message(client_fd, "INFO:-1:Connected. Send LOGIN:-1:your_name\n");
            }
        }

        for (int i = 0; i < MAX_PLAYERS; i++) {
            int client_fd = game.players[i].socket_fd;

            if (game.players[i].status != PLAYER_EMPTY &&
                client_fd >= 0 &&
                FD_ISSET(client_fd, &read_fds)) {
                char buffer[MESSAGE_BUFFER_SIZE];
                memset(buffer, 0, sizeof(buffer));

                ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

                if (bytes_read <= 0) {
                    printf("Client disconnected.\n");
                    close(client_fd);
                    remove_player(&game, i);
                    send_public_state_to_all(&game);
                } else {
                    handle_client_message(&game, client_fd, buffer);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}