#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define BUFFER_SIZE 256
#define PORT 8181
int main()
{
    int sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
    int sock_tcp = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;

    // Create socket file descriptor
    if (sock_udp < 0)
    {
        perror(" udp socket creation failed");
        exit(EXIT_FAILURE);
    }
    if (sock_tcp < 0)
    {
        perror(" tcp socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sock_udp, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("udp bind failed");
        exit(EXIT_FAILURE);
    }
    if (bind(sock_tcp, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("tcp bind failed");
        exit(EXIT_FAILURE);
    }
    listen(sock_tcp, 10);
    char buffer[BUFFER_SIZE] = {0};
    printf("\nServer Running....\n");
    fd_set readfds;
    FD_ZERO(&readfds);
    while (1)
    {
        FD_SET(sock_udp, &readfds);
        FD_SET(sock_tcp, &readfds);
        if (sock_tcp > sock_udp)
            select(sock_tcp+1, &readfds, NULL, NULL, NULL);
        else
            select(sock_udp+1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(sock_udp, &readfds)) // udp connection
        {
            struct sockaddr_in cliaddr;
            memset(&cliaddr, 0, sizeof(cliaddr));
            printf("\nUDP Connection Established\n");
            int len_client = sizeof(cliaddr);
            int packet_size = 0;
            struct hostent *h;
            if ((packet_size = recvfrom(sock_udp, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &len_client)) > 0)
            {
                // printf("Client sent %s\n", buffer);
                if ((h = gethostbyname(buffer)) == NULL)
                {
                    // printf("Error: no such host, returning 0.0.0.0 \n");
                    char *ip = "0.0.0.0\0";
                    sendto(sock_udp, (const char *)ip, strlen(ip) + 1, 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
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
                        sendto(sock_udp, (const char *)ip, strlen(ip) + 1, 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
                    }
                }
                char *end_ip = "\0";
                sendto(sock_udp, (const char *)end_ip, strlen(end_ip) + 1, 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
                printf("\n");
            }
        }
        if (FD_ISSET(sock_tcp, &readfds)) // tcp connection
        {
            struct sockaddr_in cliaddr;
            memset(&cliaddr, 0, sizeof(cliaddr));
            int len_client = sizeof(cliaddr);
            int packet_size = 0;
            struct hostent *h;
            int new_socket = accept(sock_tcp, (struct sockaddr *)&cliaddr, &len_client);
            if (fork() == 0)
            {
                close(sock_tcp);
                printf("\nTCP Connection Established\n");
                if ((packet_size = recv(new_socket, buffer, BUFFER_SIZE, 0) > 0))
                {
                    // printf("Client sent %s\n", buffer);
                    if ((h = gethostbyname(buffer)) == NULL)
                    {
                        // printf("Error: no such host, returning 0.0.0.0 \n");
                        char *ip = "0.0.0.0\0";
                        send(new_socket, (const char *)ip, strlen(ip) + 1, 0);
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
                            send(new_socket, (const char *)ip, strlen(ip) + 1, 0);
                        }
                    }
                    char *end_ip = "\0";
                    send(new_socket, (const char *)end_ip, strlen(end_ip) + 1, 0);
                    printf("\n");
                }
                close(new_socket);
                exit(0);
            }
            close(new_socket);
        }
    }
    close(sock_tcp);
    close(sock_udp);
    return 0;
}