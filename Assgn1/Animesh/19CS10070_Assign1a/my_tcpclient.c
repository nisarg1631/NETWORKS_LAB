
// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#define PORT 8080
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
			int sock = 0, valread;
			struct sockaddr_in serv_addr;
			// char *hello = "Hello from client";
			// char buffer[1024] = {0};
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
			printf("client setup done\n");
			char buffer[BUFFER_SIZE] = {0};
			int sentence_len = 0;
			int null_sent=0;  // for when number of bits % chunk size == 0
			while ((sentence_len = read(file_descriptor, buffer, CHUNK)) > 0)
			{
				if (sentence_len < CHUNK){
					buffer[sentence_len] = '\0';
					null_sent=1;
				}
				send(sock, buffer, CHUNK, 0);
			}
			if(null_sent==0)
			{
				buffer[0]='\0';
				send(sock, buffer, CHUNK, 0);
			}
			printf("sending file done\n");
			int num_sentence = 0, num_word = 0, num_char = 0;
			valread = read(sock, &num_sentence, sizeof(int));
			printf("Number of Sentences: %d\n", num_sentence);
			valread = read(sock, &num_word, sizeof(int));
			printf("Number of Words: %d\n", num_word);
			valread = read(sock, &num_char, sizeof(int));
			printf("Number of Characters: %d\n", num_char);
		}
		else
		{
			// error while opening file
			printf("File not found");
		}
	}
	return 0;
}