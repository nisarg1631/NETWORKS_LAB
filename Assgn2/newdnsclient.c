/*
	Name: Nisarg Upadhyaya
	Roll No: 19CS30031
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define MAX_BUFFER 256

int main(int argc, char const *argv[]) {
    int sockfd, serv_len;
	struct sockaddr_in serv_addr;
    struct timeval tv;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

    serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(20002);

    serv_len = sizeof(serv_addr);

    if(connect(sockfd, (struct sockaddr *)&serv_addr, serv_len) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

    char buf[MAX_BUFFER];
    printf("Enter the DNS name: ");
    scanf("%s", buf);
    send(sockfd, buf, strlen(buf)+1, 0);
    
    // recv(sockfd, &sentences, 4, 0);
    fd_set fds;
    char ip[MAX_BUFFER];
    int done = 0, timeout = 0, recvlen, currlen = 0;
    while(!done && !timeout) {
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        tv.tv_sec = 2, tv.tv_usec = 0;

        select(sockfd+1, &fds, NULL, NULL, &tv);

        if(FD_ISSET(sockfd, &fds)) {
            recvlen = recv(sockfd, buf, MAX_BUFFER, 0);
            for(int i=0; i<recvlen; i++) {
                ip[currlen++] = buf[i];
                if(buf[i] == '\0') {
                    if(currlen == 1) {
                        done = 1;
                    } else {
                        if(strcmp(ip, "0.0.0.0"))
                            printf("%s\n", ip);
                        else
                            printf("DNS name couldn't be resolved\n");
                        currlen = 0;
                    }
                }
            }
        } else {
            timeout = 1;
        }
    }

    if(done) {
        printf("Done\n");
    } else {
        printf("Timed out\n");
    }

    close(sockfd);

    return 0;
}
