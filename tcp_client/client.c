/* 
 * tcpclient.c - A simple TCP client
 * usage: tcpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <time.h>
#include "msg.h"

#define BUFSIZE 1024
#define RUNS (1024*256)

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(0);
}

int main(int argc, char **argv) {
	int sockfd, portno, n;
	struct sockaddr_in serveraddr;
	struct hostent *server;
	char *hostname;
	char buf[BUFSIZE];

    srand(time(NULL));

	/* check command line arguments */
	if (argc != 3) {
		fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
		exit(0);
	}
	hostname = argv[1];
	portno = atoi(argv[2]);

	/* socket: create the socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");

	/* gethostbyname: get the server's DNS entry */
	server = gethostbyname(hostname);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host as %s\n", hostname);
		exit(0);
	}

	/* build the server's Internet address */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, 
			(char *)&serveraddr.sin_addr.s_addr, server->h_length);
	serveraddr.sin_port = htons(portno);

	/* connect: create a connection with the server */
	if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0) 
		error("ERROR connecting");

	/* get message line from the user */
    const char alnum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	int i = 0, j = 0;
	for(; i < RUNS ; ++i) {
        unsigned long long int rand32bitA = rand() ^ (rand() << 8) ^ (rand() << 16) ^ (rand() << 24);
        unsigned long long int rand32bitB = rand() ^ (rand() << 8) ^ (rand() << 16) ^ (rand() << 24);

        msg_exch_t msg = {};
        msg.id = rand32bitA << 32 | rand32bitB;
        for(j = 0; j < 31; ++j) {
            msg.text[j] = alnum[rand() % (sizeof(alnum)-1)];
        }

        while(1) {
            printf("Write %8d/%d %x %s\n", i, RUNS, msg.id, msg.text);
            n = write(sockfd, &msg, sizeof(msg));
            if (n < 0) 
                error("ERROR writing to socket");

            bzero(buf, BUFSIZE);
            n = read(sockfd, buf, BUFSIZE);
            if (n < 0) 
                error("ERROR reading from socket");
            printf("Server response %s\n", buf);
            if(!memcmp(buf, "Fail", 4)) continue;
            if(!memcmp(buf, "OK", 2)) break;

            exit(0);
        }
    }

	close(sockfd);
	return 0;
}
