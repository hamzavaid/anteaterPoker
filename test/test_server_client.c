#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define TEST_PORT 10010
#define BUFFER_SIZE 1024

int main(void)
{
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        return 1;
    }

    server = gethostbyname("localhost");

    if (server == NULL)
    {
        fprintf(stderr, "ERROR: no such host\n");
        close(sockfd);
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    memcpy(
        &server_addr.sin_addr.s_addr,
        server->h_addr_list[0],
        server->h_length);

    server_addr.sin_port = htons(TEST_PORT);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("ERROR connecting to server");
        close(sockfd);
        return 1;
    }

    memset(buffer, 0, BUFFER_SIZE);

    if (read(sockfd, buffer, BUFFER_SIZE - 1) <= 0)
    {
        fprintf(stderr, "ERROR: did not receive initial server message\n");
        close(sockfd);
        return 1;
    }

    printf("Server says: %s", buffer);

    const char *login_msg = "LOGIN:-1:TestPlayer\n";

    if (write(sockfd, login_msg, strlen(login_msg)) < 0)
    {
        perror("ERROR writing LOGIN message");
        close(sockfd);
        return 1;
    }

    memset(buffer, 0, BUFFER_SIZE);

    if (read(sockfd, buffer, BUFFER_SIZE - 1) <= 0)
    {
        fprintf(stderr, "ERROR: did not receive LOGIN response\n");
        close(sockfd);
        return 1;
    }

    printf("Server says: %s", buffer);

    if (strstr(buffer, "SEAT:") == NULL)
    {
        fprintf(stderr, "TEST FAILED: expected SEAT response\n");
        close(sockfd);
        return 1;
    }

    printf("TEST PASSED: server accepted connection and login.\n");

    close(sockfd);
    return 0;
}