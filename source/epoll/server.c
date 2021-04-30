#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <omp.h>

#include "./wrapper/record.h"

#define TRUE 1
#define FALSE 0
#define EPOLL_QUEUE_LEN 256
#define BUFLEN 255

//Globals
int fd_server;

// Function prototypes
static void SystemFatal(const char *message);

void close_fd(int);

void print_usage(char *arg);

int do_use_fd(int fd);

int main(int argc, char *argv[])
{
	// struct Record **records;
	int current_socket_alive = 0;

	int arg, n_core;
	size_t epoll_fd;
	int port;
	struct sockaddr_in addr;

	// long options
	static struct option long_options[] = {
		{"port", required_argument, 0, 'p'},
		{"max-core", required_argument, 0, 'm'},
		{0, 0, 0, 0}};

	int long_index = 0, opt = 0;
	while ((opt = getopt_long(argc, argv, "p:m:", long_options, &long_index)) != -1)
	{
		switch (opt)
		{
		case 'p':
			port = atoi(optarg);
			break;
		case 'm':
			n_core = atoi(optarg);
			break;
		default:
			print_usage(optarg);
			exit(EXIT_FAILURE);
		}
	}

	// set up the signal handler to close the server socket when CTRL-c is received
	struct sigaction act;
	act.sa_handler = close_fd;
	act.sa_flags = 0;
	if ((sigemptyset(&act.sa_mask) == -1 || sigaction(SIGINT, &act, NULL) == -1))
	{
		perror("Failed to set SIGINT handler");
		exit(EXIT_FAILURE);
	}

	// Create the listening socket
	fd_server = socket(AF_INET, SOCK_STREAM, 0);
	if (fd_server == -1)
		SystemFatal("socket");

	// set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
	arg = 1;
	if (setsockopt(fd_server, SOL_SOCKET, (SO_REUSEADDR | SO_REUSEPORT | SO_REUSEADDR), &arg, sizeof(arg)) == -1)
		SystemFatal("setsockopt");

	// Make the server listening socket non-blocking
	if (fcntl(fd_server, F_SETFL, O_NONBLOCK | fcntl(fd_server, F_GETFL, 0)) == -1)
		SystemFatal("fcntl");

	// Bind to the specified listening port
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	if (bind(fd_server, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		SystemFatal("bind");

	// Listen for fd_news; SOMAXCONN is 128 by default
	if (listen(fd_server, SOMAXCONN) == -1)
		SystemFatal("listen");

	// Create the epoll file descriptor
	epoll_fd = epoll_create(EPOLL_QUEUE_LEN);
	if (epoll_fd == -1)
		SystemFatal("epoll_create");

	static struct epoll_event event;
	struct epoll_event events[EPOLL_QUEUE_LEN];
	size_t num_fds, fd_new;
	size_t i;

	struct sockaddr_in remote_addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);

	event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLEXCLUSIVE;
	event.data.fd = fd_server;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_server, &event) == -1)
		SystemFatal("epoll_ctl");

	omp_set_num_threads(n_core);
	int EAGAIN_REACHED;

	while (TRUE)
	{
		num_fds = epoll_wait(epoll_fd, events, EPOLL_QUEUE_LEN, -1);
		if (num_fds < 0)
			SystemFatal("Error in epoll_wait!");

		for (i = 0; i < num_fds; i++)
		{
			// Cleint disconnect condition
			if (events[i].events & EPOLLHUP)
			{
				close(events[i].data.fd);
				current_socket_alive--;
				printf("DC  alive: %d\n", current_socket_alive);
				continue;
			}

			// Error condition
			if (events[i].events & EPOLLERR)
			{
				fputs("epoll: EPOLLERR\n", stderr);
				close(events[i].data.fd);
				continue;
			}
			assert(events[i].events & EPOLLIN);

			// Server is receiving a connection request
			if (events[i].data.fd == fd_server)
			{
				EAGAIN_REACHED = 0;
				#pragma omp parallel private(fd_new)
				{
					while (!EAGAIN_REACHED)
					{
						// #pragma omp critical
						// {
						fd_new = accept(fd_server, (struct sockaddr *)&remote_addr, &addr_size);
						// }
						if (fd_new == -1)
						{
							if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
							{
								EAGAIN_REACHED = 1;
								break; // We have processed all incoming connections.
							}
							else
							{
								perror("accept");
								break;
							}
						}

						// Make the fd_new non-blocking
						if (fcntl(fd_new, F_SETFL, O_NONBLOCK | fcntl(fd_new, F_GETFL, 0)) == -1)
							SystemFatal("fcntl");

						// Add the new socket descriptor to the epoll loop
						event.data.fd = fd_new;
						if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_new, &event) == -1)
							SystemFatal("epoll_ctl");

						// Update active socket count
						current_socket_alive++;
						printf("New alive: %d\n", current_socket_alive);
					}
				}
				continue;
			}

			// Socket ready for reading
			// do_use_fd(events[i].data.fd);
			int fd = events[i].data.fd;
			if (do_use_fd(fd) == -1)
			{
				current_socket_alive--;
				printf("DC  alive: %d\n", current_socket_alive);
			}
		}
	}

	close(fd_server);
	exit(EXIT_SUCCESS);
}

int do_use_fd(int fd)
{
	char buf[BUFLEN];
	int count = 0, done = 0;
	while (1)
	{
		count = read(fd, buf, BUFLEN);
		if (count == -1)
		{
			if (errno != EAGAIN)
			{
				perror("read");
				done = 1;
			}
			break;
		}
		else if (count == 0)
		{
			done = 1;
			break;
		}
		send(fd, buf, BUFLEN, 0);
	}
	if (done)
	{
		close(fd);
		return -1;
	}
	return 0;
}

// close fd
void close_fd(int signo)
{
	close(fd_server);
	exit(EXIT_SUCCESS);
}

// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

void print_usage(char *arg)
{
	printf("Usage: %s port max_socket_allowed\n", arg);
}