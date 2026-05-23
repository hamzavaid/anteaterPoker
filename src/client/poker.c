/*
 * poker.c
 *
 * This is the main terminal-based poker client.
 *
 * Later, the GTK client GUI can reuse:
 * - client_state.c
 * - socket_client.c
 *
 * For now, this file lets us test the client-server connection
 * through the terminal.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client_state.h"
#include "socket_client.h"

/* Default server host if the user does not provide --host. */
#define DEFAULT_HOST "localhost"

/* Default server port range begins at 10010. */
#define DEFAULT_PORT 10010

/* Maximum size for user input typed into the terminal. */
#define INPUT_SIZE 256

/*
 * Parses command-line arguments for the client.
 *
 * Supported options:
 * --host <server_host>
 * --port <port_number>
 * --name <player_name>
 *
 * Example:
 * ./bin/poker --host localhost --port 10010 --name Hamza
 */
static void parse_client_args(
    int argc,
    char *argv[],
    char *host,
    int host_size,
    int *port,
    char *name,
    int name_size
)
{
    /* Set default values before reading command-line arguments. */
    snprintf(host, host_size, "%s", DEFAULT_HOST);
    snprintf(name, name_size, "Player");
    *port = DEFAULT_PORT;

    /*
     * Loop through command-line arguments.
     * Start at index 1 because argv[0] is the program name.
     */
    for (int i = 1; i < argc; i++) {
        /* Read server host. */
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            snprintf(host, host_size, "%s", argv[++i]);
        }

        /* Read server port. */
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            *port = atoi(argv[++i]);
        }

        /* Read player display name. */
        else if (strcmp(argv[i], "--name") == 0 && i + 1 < argc) {
            snprintf(name, name_size, "%s", argv[++i]);
        }
    }
}

/*
 * Handles a message received from the server.
 *
 * This function currently prints the message and updates basic ClientState fields.
 * Later, the GUI can use this same idea to update buttons, cards, labels, etc.
 */
static void handle_server_message(ClientState *client, const char *message)
{
    /* Validate inputs. */
    if (client == NULL || message == NULL) {
        return;
    }

    /* Print the raw server message for testing. */
    printf("Server says: %s", message);

    /*
     * SEAT message format:
     * SEAT:<seat_number>:Welcome <name>
     *
     * Example:
     * SEAT:0:Welcome Hamza
     */
    if (strncmp(message, "SEAT:", 5) == 0) {
        int seat = -1;

        /* Extract the seat number from the message. */
        if (sscanf(message, "SEAT:%d:", &seat) == 1) {
            client->seat = seat;
            set_client_status(client, "Logged in and seated.");
        }
    }

    /*
     * STAT message means the server sent public game state.
     * This starter version only updates the status message.
     */
    else if (strncmp(message, "STAT:", 5) == 0) {
        set_client_status(client, "Received public game state.");
    }

    /*
     * HAND message means the server sent this player's private hand.
     * This starter version only updates the status message.
     */
    else if (strncmp(message, "HAND:", 5) == 0) {
        set_client_status(client, "Received private hand.");
    }

    /*
     * ERROR message means the server rejected something.
     */
    else if (strncmp(message, "ERROR:", 6) == 0) {
        set_client_status(client, "Server returned an error.");
    }

    /*
     * OK message means the server accepted a client action.
     */
    else if (strncmp(message, "OK:", 3) == 0) {
        set_client_status(client, "Action accepted.");
    }
}

/*
 * Prints the available terminal commands.
 */
static void print_help(void)
{
    printf("\nCommands:\n");
    printf("  start              Start a new hand\n");
    printf("  check              Send CHECK action\n");
    printf("  call               Send CALL action\n");
    printf("  fold               Send FOLD action\n");
    printf("  raise <amount>     Send RAISE action\n");
    printf("  state              Print local client state\n");
    printf("  help               Show commands\n");
    printf("  quit               Disconnect\n\n");
}

/*
 * Converts a user terminal command into a server protocol message.
 *
 * Example:
 * User types:
 * check
 *
 * Client sends:
 * ACTN:<seat>:CHECK
 */
static void build_action_message(
    const ClientState *client,
    const char *input,
    char *message,
    int message_size
)
{
    /* Validate inputs. */
    if (client == NULL || input == NULL || message == NULL) {
        return;
    }

    /* Start with an empty output message. */
    message[0] = '\0';

    /*
     * Send START command.
     * This tells the server to begin a new hand.
     */
    if (strncmp(input, "start", 5) == 0) {
        snprintf(message, message_size, "START:%d:\n", client->seat);
    }

    /*
     * Send CHECK action.
     */
    else if (strncmp(input, "check", 5) == 0) {
        snprintf(message, message_size, "ACTN:%d:CHECK\n", client->seat);
    }

    /*
     * Send CALL action.
     */
    else if (strncmp(input, "call", 4) == 0) {
        snprintf(message, message_size, "ACTN:%d:CALL\n", client->seat);
    }

    /*
     * Send FOLD action.
     */
    else if (strncmp(input, "fold", 4) == 0) {
        snprintf(message, message_size, "ACTN:%d:FOLD\n", client->seat);
    }

    /*
     * Send RAISE action.
     *
     * Expected user format:
     * raise 50
     */
    else if (strncmp(input, "raise", 5) == 0) {
        int amount = 0;

        /* Try to read the raise amount after the word "raise". */
        if (sscanf(input, "raise %d", &amount) == 1) {
            snprintf(message, message_size, "RAISE:%d:%d\n", client->seat, amount);
        } else {
            printf("Usage: raise <amount>\n");
        }
    }

    /*
     * Send QUIT command.
     * This tells the server the client is leaving.
     */
    else if (strncmp(input, "quit", 4) == 0) {
        snprintf(message, message_size, "QUIT:%d:\n", client->seat);
    }
}

/*
 * Program entry point for the terminal poker client.
 */
int main(int argc, char *argv[])
{
    /* Stores all local client information. */
    ClientState client;

    /* Server host name or IP address. */
    char host[128];

    /* Player display name. */
    char name[CLIENT_NAME_LEN];

    /* Buffer for messages received from the server. */
    char buffer[CLIENT_BUFFER_SIZE];

    /* Buffer for user input from the terminal. */
    char input[INPUT_SIZE];

    /* Buffer for messages sent to the server. */
    char outgoing[CLIENT_BUFFER_SIZE];

    /* Server port number. */
    int port;

    /* Initialize local client state. */
    init_client_state(&client);

    /* Read command-line options or use defaults. */
    parse_client_args(
        argc,
        argv,
        host,
        sizeof(host),
        &port,
        name,
        sizeof(name)
    );

    /* Store the player's name in ClientState. */
    set_client_name(&client, name);

    /* Show connection information. */
    printf("Connecting to server...\n");
    printf("Host: %s\n", host);
    printf("Port: %d\n", port);
    printf("Name: %s\n", name);

    /* Try to connect to the server. */
    client.socket_fd = connect_to_server(host, port);

    /* Stop if the connection failed. */
    if (client.socket_fd < 0) {
        fprintf(stderr, "Could not connect to server.\n");
        return 1;
    }

    /* Mark the client as connected. */
    client.connected = 1;

    /* Update status message. */
    set_client_status(&client, "Connected to server.");

    /*
     * The server should send an INFO message immediately after connection.
     */
    if (receive_from_server(client.socket_fd, buffer, sizeof(buffer)) > 0) {
        handle_server_message(&client, buffer);
    }

    /*
     * Send LOGIN message to the server.
     *
     * Format:
     * LOGIN:-1:<player_name>
     *
     * -1 is used because the server has not assigned a seat yet.
     */
    snprintf(outgoing, sizeof(outgoing), "LOGIN:-1:%s\n", client.player_name);
    send_to_server(client.socket_fd, outgoing);

    /*
     * Receive server response to LOGIN.
     * Expected response:
     * SEAT:<seat_number>:Welcome <name>
     */
    if (receive_from_server(client.socket_fd, buffer, sizeof(buffer)) > 0) {
        handle_server_message(&client, buffer);
    }

    /* Show the available terminal commands. */
    print_help();

    /*
     * Main client input loop.
     *
     * This loop waits for the user to type commands,
     * sends messages to the server,
     * and prints server responses.
     */
    while (1) {
        /* Print prompt. */
        printf("poker> ");

        /* Read input from the user. */
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        /* Remove the newline character from input. */
        input[strcspn(input, "\n")] = '\0';

        /* Show command list. */
        if (strcmp(input, "help") == 0) {
            print_help();
            continue;
        }

        /* Print local state. */
        if (strcmp(input, "state") == 0) {
            print_client_state(&client);
            continue;
        }

        /* Convert user input into a server protocol message. */
        build_action_message(&client, input, outgoing, sizeof(outgoing));

        /* If no message was created, the command was unknown. */
        if (outgoing[0] == '\0') {
            printf("Unknown command. Type 'help' for options.\n");
            continue;
        }

        /* Send the command to the server. */
        send_to_server(client.socket_fd, outgoing);

        /*
         * If the user chose quit, leave the loop after sending QUIT.
         */
        if (strncmp(input, "quit", 4) == 0) {
            break;
        }

        /*
         * Receive the server response after sending an action.
         */
        if (receive_from_server(client.socket_fd, buffer, sizeof(buffer)) > 0) {
            handle_server_message(&client, buffer);
        } else {
            printf("Server disconnected.\n");
            break;
        }
    }

    /* Close the socket connection. */
    close(client.socket_fd);

    /* Mark client as disconnected. */
    client.connected = 0;

    printf("Disconnected from server.\n");

    return 0;
}