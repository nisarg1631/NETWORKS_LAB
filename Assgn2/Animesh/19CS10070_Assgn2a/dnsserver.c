#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define BUFFER_SIZE 256

int main()
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    // Create socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(8181);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr,
             sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    char buffer[BUFFER_SIZE] = {0};
    printf("\nServer Running....\n");
    while (1)
    {
        int len_client = sizeof(cliaddr);
        int packet_size = 0;
        struct hostent *h;

        if ((packet_size = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &len_client)) > 0)
        {
            // printf("Client sent %s\n", buffer);
            if ((h = gethostbyname(buffer)) == NULL)
            {
                // printf("Error: no such host, returning 0.0.0.0 \n");
                char *ip = "0.0.0.0\0";
                sendto(sockfd, (const char *)ip, strlen(ip) + 1, 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
            }
            else
            {
                // get the host info
                // printf("Host name : %s\n", h->h_name);
                // printf("All addresses: ");
                struct in_addr **addr_list = (struct in_addr **)h->h_addr_list;
                for (int i = 0; addr_list[i] != NULL; i++)
                {
                    // printf("%s, ", inet_ntoa(*addr_list[i]));
                    char *ip = inet_ntoa(*addr_list[i]);
                    sendto(sockfd, (const char *)ip, strlen(ip) + 1, 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
                }
            }
            char *end_ip = "\0";
            sendto(sockfd, (const char *)end_ip, strlen(end_ip) + 1, 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
            printf("\n");
        }
    }
    return 0;
}