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

#define MAX_BUFFER 256

int main() {
    int sockfd, cli_len;
	struct sockaddr_in cli_addr, serv_addr;
    char buf[MAX_BUFFER];

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(20000);

	if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

	printf("UDP Server Running...\n");
	int processed = 1;

	while(1) {

		cli_len = sizeof(cli_addr);
        int recvlen = recvfrom(sockfd, buf, MAX_BUFFER, 0, (struct sockaddr *)&cli_addr, &cli_len);

        printf("DNS name received: %s\n", buf);

        struct hostent *he;
        struct in_addr **addr_list;
        if((he = gethostbyname(buf)) == NULL || *(addr_list = (struct in_addr **)he->h_addr_list) == NULL) {
            strcpy(buf, "0.0.0.0");
            sendto(sockfd, buf, strlen(buf)+1, 0, (const struct sockaddr *)&cli_addr, cli_len);
        } else {
            for(int i = 0; addr_list[i] != NULL; i++) {
                strcpy(buf, inet_ntoa(*addr_list[i]));
                sendto(sockfd, buf, strlen(buf)+1, 0, (const struct sockaddr *)&cli_addr, cli_len);
            }
        }
		
        buf[0] = '\0';
        sendto(sockfd, buf, strlen(buf)+1, 0, (const struct sockaddr *)&cli_addr, cli_len);
		printf("Total requests processed: %d\n", processed++);
	}

    return 0;
}
