#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "./wrapper/record.h"

#define BUFLEN 255           //Buffer length
#define TRUE 1
#define LISTENQ 5

// Function Prototypes
static void SystemFatal(const char *);
void print_usage(char *arg);

int main(int argc, char **argv)
{
    // struct Record **records;

    size_t max_socket_alive = 0;
    size_t current_socket_alive = 0;

    size_t i, maxi, nready, arg;
    int listen_sd, new_sd, sockfd, port, maxfd, client[FD_SETSIZE];
    unsigned int client_len;
    struct sockaddr_in server, client_addr;
    char buf[BUFLEN];
    ssize_t n;
    fd_set rset, allset;

    // long options
    static struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {0, 0, 0, 0}};

    int long_index = 0, opt = 0;
    while ((opt = getopt_long(argc, argv, "p:", long_options, &long_index)) != -1)
    {
        switch (opt)
        {
        case 'p':
            port = atoi(optarg);
            break;
        default:
            print_usage(optarg);
            exit(EXIT_FAILURE);
        }
    }

    // Create a stream socket
    if ((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        SystemFatal("Cannot Create Socket!");

    // set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
    arg = 1;
    if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
        SystemFatal("setsockopt");

    // Bind an address to the socket
    bzero((char *)&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client

    if (bind(listen_sd, (struct sockaddr *)&server, sizeof(server)) == -1)
        SystemFatal("bind error");

    // Listen for connections
    // queue up to LISTENQ connect requests
    listen(listen_sd, LISTENQ);

    maxfd = listen_sd; // initialize
    maxi = -1;         // index into client[] array

    for (i = 0; i < FD_SETSIZE; i++)
        client[i] = -1; // -1 indicates available entry
    FD_ZERO(&allset);
    FD_SET(listen_sd, &allset);

    while (TRUE)
    {
        rset = allset; // structure assignment
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listen_sd, &rset)) // new client connection
        {
            client_len = sizeof(client_addr);
            if ((new_sd = accept(listen_sd, (struct sockaddr *)&client_addr, &client_len)) == -1)
                SystemFatal("accept error");

            current_socket_alive++;
            if (current_socket_alive > max_socket_alive)
            {
                max_socket_alive = current_socket_alive;
                printf("New max socket alive %ld\n", max_socket_alive);
            }

            for (i = 0; i < FD_SETSIZE; i++)
                if (client[i] < 0)
                {
                    client[i] = new_sd; // save descriptor
                    break;
                }
            if (i == FD_SETSIZE)
            {
                printf("Too many clients\n");
                exit(1);
            }

            FD_SET(new_sd, &allset); // add new descriptor to set
            if (new_sd > maxfd)
                maxfd = new_sd; // for select

            if (i > maxi)
                maxi = i; // new max index in client[] array

            if (--nready <= 0)
                continue; // no more readable descriptors
        }

        for (i = 0; i <= maxi; i++) // check all clients for data
        {
            if ((sockfd = client[i]) < 0)
                continue;

            if (FD_ISSET(sockfd, &rset))
            {
                if ((n = read(sockfd, buf, BUFLEN)) == 0)
                {
                    current_socket_alive--;
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                }
                else
                {
                    write(sockfd, buf, BUFLEN); // echo to client
                }

                if (--nready <= 0)
                    break; // no more readable descriptors
            }
        }
    }

    return (0);
}

// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

void print_usage(char *arg)
{
	printf("Usage: %s port max-socket-allowed\n", arg);
}