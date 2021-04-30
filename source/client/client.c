#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <omp.h>
#include <time.h>

#include "client.h"

void print_usage(char *arg)
{
	printf("Usage: %s host [port]\n", arg);
}

void interrupt_handler(int signo);

void delay(long delay);

int main(int argc, char **argv)
{
	int i;
	int sd, send_count, port;
	struct hostent *hp;
	struct sockaddr_in server;
	char *host, *msg, sbuf[BUFLEN], rbuf[BUFLEN];

	long delay_nsec = 0;
	struct timespec start, end;
	unsigned long elapsed = 0;

	// set up the signal handler to close the server socket when CTRL-c is received
	struct sigaction act;
	act.sa_handler = interrupt_handler;
	act.sa_flags = 0;
	if ((sigemptyset(&act.sa_mask) == -1 || sigaction(SIGINT, &act, NULL) == -1))
	{
		perror("Failed to set SIGINT handler");
		exit(EXIT_FAILURE);
	}

	// long options
	static struct option long_options[] = {
		{"host", required_argument, 0, 'h'},
		{"port", required_argument, 0, 'p'},
		{"message", required_argument, 0, 'm'},
		{"count", required_argument, 0, 'c'},
		{"delay", required_argument, 0, 'd'},
		{0, 0, 0, 0}};

	int long_index = 0, opt = 0;
	while ((opt = getopt_long(argc, argv, "h:p:m:c:d:", long_options, &long_index)) != -1)
	{
		switch (opt)
		{
		case 'h':
			host = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'm':
			msg = optarg;
			strcpy(sbuf, msg);
			break;
		case 'c':
			send_count = atoi(optarg);
			break;
		case 'd':
			delay_nsec = atol(optarg);
			break;
		default:
			print_usage(optarg);
			exit(EXIT_FAILURE);
		}
	}

	// Create the socket list
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Cannot create socket");
		exit(1);
	}

	// Initialize server info
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if ((hp = gethostbyname(host)) == NULL)
	{
		fprintf(stderr, "Unknown server address\n");
		exit(1);
	}
	bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);

	// Connecting to the server
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		fprintf(stderr, "Can't connect to server\n");
		perror("connect");
		exit(1);
	}

	for (i = 0; i < send_count; i++)
	{
		send(sd, sbuf, BUFLEN, 0);
		// printf("transmit: %s\n", sbuf);
		clock_gettime(CLOCK_REALTIME, &start);
		recv(sd, rbuf, BUFLEN, 0);
		clock_gettime(CLOCK_REALTIME, &end);
		elapsed += (end.tv_nsec - start.tv_nsec) + (end.tv_sec - start.tv_sec) * 1000000000;
		// printf("receive : %s\n", rbuf);
		delay(delay_nsec);
	}

	printf("request count         : %d\ndata transfer         : %ld byte\nresponse time         : %f sec\naverage response time : %f sec\n\n"
	,send_count, send_count * strlen(msg), elapsed / (float) 1000000000, elapsed / (float) send_count / (float)1000000000);

	close(sd);
	fflush(stdout);
	return (0);
}

void delay(long delay_nsec)
{
	long passed = 0;
	if (delay_nsec)
	{
		struct timespec start, current;
		clock_gettime(CLOCK_REALTIME, &start);

		while (passed <= delay_nsec)
		{
			clock_gettime(CLOCK_REALTIME, &current);
			if ((current.tv_sec - start.tv_sec) >= 1){
				return;
			}
			passed = current.tv_nsec - start.tv_nsec;
		}
	}
}

void interrupt_handler(int signo)
{
	fprintf(stderr, "Interrupt\n");
	// perror("FUCK\n");
}