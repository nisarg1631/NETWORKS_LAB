// Server side C/C++ program to demonstrate Socket programming
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#define PORT 8080
#define BUFFER_SIZE 100
int main(int argc, char const *argv[])
{
	// sample code
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[BUFFER_SIZE] = {0};
	// char *hello = "Hello from server";

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 8080
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
				   &opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	// Forcefully attaching socket to the port 8080
	if (bind(server_fd, (struct sockaddr *)&address,
			 sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	while (1) // first change in sample code
	{
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
								 (socklen_t *)&addrlen)) < 0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}
		printf("Connection accepted\n");
		int num_sentences = 0;
		int num_words = 0;
		int num_chars = 0;
		int flag = 0;
		int packet_size = 0;
		int white_space_seen = 0;
		int len_word_cur = 0;
		while ((packet_size = read(new_socket, buffer, BUFFER_SIZE)) > 0)
		{
			// printf("%s\n", buffer);
			// printf("bytes = %d\n", packet_size);
			for (int i = 0; i < packet_size; i++)
			{
				// printf("%c\n", buffer[i]);
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
		send(new_socket, &num_sentences, sizeof(int), 0);
		send(new_socket, &num_words, sizeof(int), 0);
		send(new_socket, &num_chars, sizeof(int), 0);
		close(new_socket);
		printf("Done\n");
	}
	printf("server shutdown\n");
	return 0;
}