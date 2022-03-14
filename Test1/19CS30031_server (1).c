/*
	Name: Nisarg Upadhyaya
	Roll No: 19CS30031
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#define MAX_BUFFER 256
#define MAX_QUEUE 5
#define MAX_CLIENTS 3

struct client_info{
	struct sockaddr_in cli_addr;
	int cli_len;
};

void recvNbytes(int sockfd, char buf[], int n) {
    recv(sockfd, buf, n, MSG_WAITALL);
}

int max(int n1, int n2) {
    return (n1>n2) ? n1 : n2;
}

int main() {
    int tcpsockfd, clientsockfd[MAX_CLIENTS];
	for(int i=0; i<MAX_CLIENTS; i++)
		clientsockfd[i] = -1;
	struct sockaddr_in tcpserv_addr;
	struct client_info clientId[MAX_CLIENTS];
	int numclient = 0;

	if((tcpsockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("TCP -- Cannot create socket\n");
		exit(0);
	}

	tcpserv_addr.sin_family = AF_INET;
	tcpserv_addr.sin_addr.s_addr = INADDR_ANY;
	tcpserv_addr.sin_port = htons(20002);

	if(bind(tcpsockfd, (struct sockaddr *)&tcpserv_addr, sizeof(tcpserv_addr)) < 0) {
		perror("TCP -- Unable to bind local address\n");
		exit(0);
	}

	listen(tcpsockfd, MAX_QUEUE);

	printf("TCP server running...\n");

	fd_set fds;
        
	while(1) {
		FD_ZERO(&fds);
		FD_SET(tcpsockfd, &fds);
		int maxfd = tcpsockfd;
		for(int i=0; i<MAX_CLIENTS; i++) {
			if(clientsockfd[i] != -1) {
				FD_SET(clientsockfd[i], &fds);
				maxfd = max(maxfd, clientsockfd[i]);
			}
		}
		// printf("Hiiii\n");
		fflush(stdout);
		select(maxfd+1, &fds, NULL, NULL, NULL);
		// printf("Hiiii2\n");
		fflush(stdout);
		if(FD_ISSET(tcpsockfd, &fds)) {
			clientId[numclient].cli_len = sizeof(clientId[numclient].cli_addr);
            if ((clientsockfd[numclient] = accept(tcpsockfd, (struct sockaddr *)&clientId[numclient].cli_addr, &clientId[numclient].cli_len)) < 0) {
                perror("TCP -- Accept error\n");
                exit(0);
            }
			printf("Server: Received a new connection from client <%s: %d>\n", inet_ntoa(clientId[numclient].cli_addr.sin_addr), clientId[numclient].cli_addr.sin_port);
			numclient++;
		}
		for(int i=0; i<MAX_CLIENTS; i++) {
			if(clientsockfd[i] != -1 && FD_ISSET(clientsockfd[i], &fds)) {
				fflush(stdout);
				char buf[MAX_BUFFER];
				char message[MAX_BUFFER];
				int meslen = 0;
				int done = 0;
				while(!done) {
					int recvlen = recv(clientsockfd[i], buf, MAX_BUFFER, 0);
					for(int j=0; j<recvlen; j++) {
						message[meslen++] = buf[j];
						if(buf[j] == '\0') {
							done = 1;
							break;
						}
					}
				}
				printf("Ok3333\n");
				fflush(stdout);
				printf("Server: Received message \"%s\" from client <%s: %d>\n", message, inet_ntoa(clientId[i].cli_addr.sin_addr), clientId[i].cli_addr.sin_port);
				if(numclient == 1) {
					printf("Server: Insufficient clients, \"%s\" from client <%s: %d> dropped\n", message, inet_ntoa(clientId[i].cli_addr.sin_addr), clientId[i].cli_addr.sin_port);
				} else {
					for(int j=0; j<MAX_CLIENTS; j++) {
						if(clientsockfd[j] != -1 && j != i) {
							uint32_t iptemp = htonl(clientId[j].cli_addr.sin_addr.s_addr);
							send(clientsockfd[j], &iptemp, 4, 0);
							uint16_t porttemp = htons(clientId[j].cli_addr.sin_port);
							send(clientsockfd[j], &porttemp, 2, 0);
							send(clientsockfd[j], message, meslen, 0);
							printf("Server: Sent message \"%s\" from client <%s: %d> to <%s: %d>\n", message, inet_ntoa(clientId[i].cli_addr.sin_addr), clientId[i].cli_addr.sin_port, inet_ntoa(clientId[j].cli_addr.sin_addr), clientId[j].cli_addr.sin_port);
						}
					}
				}
			}
		}
	}

    return 0;
}
