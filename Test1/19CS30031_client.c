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
#include <errno.h>

#define MAX_BUFFER 256

int main(int argc, char const *argv[]) {
    int sockfd, serv_len;
	struct sockaddr_in serv_addr;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

    serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(20002);

    serv_len = sizeof(serv_addr);

    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);

    int N = 1;

    while(connect(sockfd, (struct sockaddr *)&serv_addr, serv_len) < 0) {
		if(errno == EINPROGRESS || errno == EALREADY) {
            sleep(1);
        } else {
		    exit(0);
        }
	}
    while(1) {
        int sleeptime = rand();
        sleep((sleeptime%3) + 1);
        if(N < 6) {
            char buf[MAX_BUFFER];
            sprintf(buf, "Message %d", N);
            buf[9] = '\0';
            send(sockfd, buf, 10, 0);
            printf("Message %d sent\n", N);
            N++;
        }
        uint32_t netip;
        uint16_t netport;
        while (recv(sockfd, &netip, 4, 0) < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                sleep(1);
            else {
                printf("Error in recv\n");
                exit(0);
            }
        }
        while (recv(sockfd, &netport, 2, 0) < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                sleep(1);
            else {
                printf("Error in recv\n");
                exit(0);
            }
        }
        char buf[MAX_BUFFER];
        while (recv(sockfd, buf, 10, 0) < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                sleep(1);
            else {
                printf("Error in recv\n");
                exit(0);
            }
        }
        struct in_addr temp;
        temp.s_addr = ntohl(netip);
        printf("Client: Received %s from <%s: %d>\n", buf, inet_ntoa(temp), ntohs(netport));
    }

    return 0;
}
