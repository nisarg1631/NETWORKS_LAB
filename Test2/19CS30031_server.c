#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>

#define MAX_QUEUE 5
#define MAX_BUF 1000

void recvInt(int sockfd, int *i) {
    recv(sockfd, i, 4, MSG_WAITALL);
}

void recvNullTerm(int sockfd, char buf[], int *len) {
    *len = 0;
    char temp;
    while(recv(sockfd, &temp, 1, MSG_WAITALL)) {
        buf[(*len)++] = temp;
        if(temp == '\0')
            break;
    }
}

void *consumer(void *param) {
    int *sockfd = (int *)param;
    char command[MAX_BUF];
    int commandlen = 0;
    recvNullTerm(*sockfd, command, &commandlen);
    printf("%s\n%d\n", command, commandlen);
    char filename[MAX_BUF];
    int filenamelen = 0;
    recvNullTerm(*sockfd, filename, &filenamelen);
    printf("%s\n%d\n", filename, filenamelen);
    if(!strcmp(command, "del")) {
        printf("Delete comm\n");
    } else {
        printf("Access comm\n");
        int x, y;
        recvInt(*sockfd, &x);
        recvInt(*sockfd, &y);
        printf("%d %d\n", x, y);
    }
    close(*sockfd);
    pthread_exit(0);
}

signed main() {
    // tcp
    int tcpsockfd, newtcpsockfd, tcpcli_len;
	struct sockaddr_in tcpcli_addr, tcpserv_addr;

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

	printf("TCP server Running...\n");

    while(1) {
        int clilen = sizeof(tcpcli_addr);
        int *newsockfd = (int *)malloc(sizeof(int));
		if ((*newsockfd = accept(tcpsockfd, (struct sockaddr *)&tcpcli_addr, &clilen)) < 0) {
			perror("Accept error\n");
			exit(0);
		}
        pthread_t clihandler;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&clihandler, &attr, consumer, (void *)newsockfd);
    }
    return 0;
}
