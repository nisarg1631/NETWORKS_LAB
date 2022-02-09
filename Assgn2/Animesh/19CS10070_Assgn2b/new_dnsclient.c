#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#define PORT 8181
#define BUFFER_SIZE 255
int main(int argc, char const *argv[])
{
    // code from sample
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    // setup complete
    // printf("tcp client setup done\n");

    int len = sizeof(serv_addr);
    char buffer[BUFFER_SIZE] = {0};
    printf("Enter DNS name\n");
    scanf("%s", buffer);
    send(sock, buffer, strlen(buffer) + 1, 0);
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
        FD_SET(sock, &readfds);
        int retval = select(sock + 1, &readfds, NULL, NULL, &tv);
        if (retval == -1)
        {
            printf("select failed\n");
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(sock, &readfds))
        {
            int prev_null = 0;
            int current_len = 0;

            while ((packet_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0)
            {
                for (int i = 0; i < packet_size; i++)
                {
                    if (buffer[i] == '\0')
                    {
                        if (prev_null == 0)
                        {
                            prev_null = 1;
                            if (strcmp(buffer + i - current_len, "0.0.0.0\0") == 0)
                                printf("Error: no ip found for this dns name\n");
                            else
                                printf("%s\n", buffer + i - current_len);
                            current_len = 0;
                        }
                        else
                            break;
                    }
                    else
                    {
                        prev_null = 0;
                        current_len++;
                    }
                }
            }
            flag = 1;
        }
        else
        {
            timeout = 1;
            break;
        }
    }
    if (timeout)
        printf("timeout\n");
    else
        printf("done\n");
    close(sock);

    return 0;
}