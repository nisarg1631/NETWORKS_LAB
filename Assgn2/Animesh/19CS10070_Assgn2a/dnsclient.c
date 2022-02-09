// A Simple Client Implementation
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#define PORT 8181
#define BUFFER_SIZE 256
int main(int argc, char const *argv[])
{

    int sockfd;
    struct sockaddr_in servaddr;
    // Creating socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    int len = sizeof(servaddr);
    char buffer[BUFFER_SIZE] = {0};
    printf("Enter DNS name\n");
    scanf("%s", buffer);
    sendto(sockfd, buffer, strlen(buffer) + 1, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    // printf("sending DNS name done\n");

    int packet_size = 0;
    int flag = 0;
    int timeout = 0;
    fd_set readfds;
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    while (!(timeout || flag))
    {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        int retval = select(sockfd + 1, &readfds, NULL, NULL, &tv);
        if (~retval == 0)
        {
            printf("select failed\n");
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(sockfd, &readfds))
        {
            while ((packet_size = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&servaddr, &len)) > 0)
            {
                if (buffer[0] == '\0')
                    break;
                else if (strcmp(buffer, "0.0.0.0\0") == 0)
                    printf("Error: no ip found for this dns name\n");
                else
                    printf("%s\n", buffer);
            }
            flag = 1;
        }
        else{
            timeout=1;
            break;
        }
    }
    if (timeout)
        printf("timeout\n");
    else
        printf("done\n");
    close(sockfd);
    return 0;
}