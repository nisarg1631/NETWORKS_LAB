// A Simple UDP Server that sends a HELLO message
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include<ctype.h>
#define MAXLINE 1024
#define BUFFER_SIZE 100
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
        int len_client=sizeof(cliaddr);
        int num_sentences = 0;
		int num_words = 0;
		int num_chars = 0;
		int flag = 0;
		int packet_size = 0;
		int white_space_seen = 0;
		int len_word_cur = 0;
		while ((packet_size = recvfrom(sockfd, buffer, BUFFER_SIZE,0,(struct sockaddr*)&cliaddr,&len_client)) > 0)
		{
			for (int i = 0; i < packet_size; i++)
			{
				if (buffer[i] == '\0')
				{
					flag = 1;
					break;
				}
				num_chars++;
				if (buffer[i] == '.')
				{
					if (len_word_cur > 0)
					{
						num_words++;
						len_word_cur = 0;
					}
					num_sentences++;
				}
				else if (isalnum(buffer[i]))
				{
					len_word_cur++;
					if (white_space_seen)
					{
						white_space_seen = 0;
					}
				}
				else if (white_space_seen == 0)
				{
					if (len_word_cur > 0)
					{
						num_words++;
						len_word_cur = 0;
					}
					white_space_seen = 1;
				}
			}
			if (flag)
			{
				break;
			}
		}
		printf("processing done\n");
		printf("num_sentences = %d\n", num_sentences);
		printf("num_words = %d\n", num_words);
		printf("num_chars = %d\n", num_chars);
		sendto(sockfd, &num_sentences, sizeof(int), 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
		sendto(sockfd, &num_words, sizeof(int), 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
		sendto(sockfd, &num_chars, sizeof(int), 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));
		// close(sockfd);
		printf("Done\n");
    }
    return 0;
}