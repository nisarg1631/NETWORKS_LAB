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

int max(int n1, int n2) {
    return (n1>n2) ? n1 : n2;
}

int main() {
    // tcp
    int tcpsockfd, newtcpsockfd, tcpcli_len;
	struct sockaddr_in tcpcli_addr, tcpserv_addr;

    // udp
    int udpsockfd, udpcli_len;
	struct sockaddr_in udpcli_addr, udpserv_addr;

    fd_set fds;

    char tcpbuf[MAX_BUFFER], udpbuf[MAX_BUFFER];

	if((tcpsockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("TCP -- Cannot create socket\n");
		exit(0);
	}

    if((udpsockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("UDP -- Cannot create socket\n");
		exit(0);
	}

	tcpserv_addr.sin_family = AF_INET;
	tcpserv_addr.sin_addr.s_addr = INADDR_ANY;
	tcpserv_addr.sin_port = htons(20002);

    udpserv_addr.sin_family = AF_INET;
	udpserv_addr.sin_addr.s_addr = INADDR_ANY;
	udpserv_addr.sin_port = htons(20000);

	if(bind(tcpsockfd, (struct sockaddr *)&tcpserv_addr, sizeof(tcpserv_addr)) < 0) {
		perror("TCP -- Unable to bind local address\n");
		exit(0);
	}

    if(bind(udpsockfd, (struct sockaddr *)&udpserv_addr, sizeof(udpserv_addr)) < 0) {
		perror("UDP -- Unable to bind local address\n");
		exit(0);
	}

	listen(tcpsockfd, MAX_QUEUE);

	printf("TCP and UDP server Running...\n");
	int tcpprocessed = 1, udpprocessed = 1;
        
	while(1) {
        FD_ZERO(&fds);
        FD_SET(tcpsockfd, &fds);
        FD_SET(udpsockfd, &fds);

        select(max(tcpsockfd, udpsockfd)+1, &fds, NULL, NULL, NULL);

        if(FD_ISSET(tcpsockfd, &fds)) {
            tcpcli_len = sizeof(tcpcli_addr);
            if ((newtcpsockfd = accept(tcpsockfd, (struct sockaddr *)&tcpcli_addr, &tcpcli_len)) < 0) {
                perror("TCP -- Accept error\n");
                exit(0);
            }

            if(fork() == 0) {
                int recvlen = recv(newtcpsockfd, tcpbuf, MAX_BUFFER, 0);

                printf("TCP -- DNS name received: %s\n", tcpbuf);

                struct hostent *he;
                struct in_addr **addr_list;
                if((he = gethostbyname(tcpbuf)) == NULL || *(addr_list = (struct in_addr **)he->h_addr_list) == NULL) {
                    strcpy(tcpbuf, "0.0.0.0");
                    send(newtcpsockfd, tcpbuf, strlen(tcpbuf)+1, 0);
                } else {
                    for(int i = 0; addr_list[i] != NULL; i++) {
                        strcpy(tcpbuf, inet_ntoa(*addr_list[i]));
                        send(newtcpsockfd, tcpbuf, strlen(tcpbuf)+1, 0);
                    }
                }
                
                tcpbuf[0] = '\0';
                send(newtcpsockfd, tcpbuf, strlen(tcpbuf)+1, 0);
                
                printf("TCP -- Total requests processed: %d\n", tcpprocessed);
                close(newtcpsockfd);
                exit(0);
            }

            close(newtcpsockfd);
            tcpprocessed++;
        }

        if(FD_ISSET(udpsockfd, &fds)) {
            udpcli_len = sizeof(udpcli_addr);
            int recvlen = recvfrom(udpsockfd, udpbuf, MAX_BUFFER, 0, (struct sockaddr *)&udpcli_addr, &udpcli_len);

            printf("UDP -- DNS name received: %s\n", udpbuf);

            struct hostent *he;
            struct in_addr **addr_list;
            if((he = gethostbyname(udpbuf)) == NULL || *(addr_list = (struct in_addr **)he->h_addr_list) == NULL) {
                strcpy(udpbuf, "0.0.0.0");
                sendto(udpsockfd, udpbuf, strlen(udpbuf)+1, 0, (const struct sockaddr *)&udpcli_addr, udpcli_len);
            } else {
                for(int i = 0; addr_list[i] != NULL; i++) {
                    strcpy(udpbuf, inet_ntoa(*addr_list[i]));
                    sendto(udpsockfd, udpbuf, strlen(udpbuf)+1, 0, (const struct sockaddr *)&udpcli_addr, udpcli_len);
                }
            }
            
            udpbuf[0] = '\0';
            sendto(udpsockfd, udpbuf, strlen(udpbuf)+1, 0, (const struct sockaddr *)&udpcli_addr, udpcli_len);
            printf("UDP -- Total requests processed: %d\n", udpprocessed++);
        }
	}

    return 0;
}
