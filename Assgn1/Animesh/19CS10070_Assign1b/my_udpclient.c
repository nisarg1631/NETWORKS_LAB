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
#define PORT 8181
#define BUFFER_SIZE 100
#define CHUNK 64
int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        // if no cmd line arg passed
        printf("no file specified");
    }
    else
    {
        int file_descriptor = 0;
        if ((file_descriptor = open(argv[1], O_RDONLY)) >= 0)
        {
            // if file opened successfully

            // code from sample
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

            // int n;
            // socklen_t len;
            // char *hello = "CLIENT:HELLO";

            // sendto(sockfd, (const char *)hello, strlen(hello), 0,
            //        (const struct sockaddr *)&servaddr, sizeof(servaddr));
            // printf("Hello message sent from client\n");
            int len = sizeof(servaddr);
            char buffer[BUFFER_SIZE] = {0};
            int sentence_len = 0;
            int null_sent = 0; // for when number of bits % chunk size == 0
            while ((sentence_len = read(file_descriptor, buffer, CHUNK)) > 0)
            {
                if (sentence_len < CHUNK)
                {
                    null_sent = 1;
                    buffer[sentence_len] = '\0';
                }
                sendto(sockfd, buffer, CHUNK, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
            }

            if (null_sent == 0)
            {
                buffer[0] = '\0';
                sendto(sockfd, buffer, CHUNK, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
            }
            printf("sending file done\n");
            int num_sentence = 0, num_word = 0, num_char = 0;
            recvfrom(sockfd, &num_sentence, sizeof(int), 0, (struct sockaddr *)&servaddr, &len);
            printf("Number of Sentences: %d\n", num_sentence);
            recvfrom(sockfd, &num_word, sizeof(int), 0, (struct sockaddr *)&servaddr, &len);
            printf("Number of Words: %d\n", num_word);
            recvfrom(sockfd, &num_char, sizeof(int), 0, (struct sockaddr *)&servaddr, &len);
            printf("Number of Characters: %d\n", num_char);
            close(sockfd);
        }
        else
        {
            // error while opening file
            printf("File not found");
        }
    }
    return 0;
}