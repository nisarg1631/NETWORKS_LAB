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

#define MAX_QUEUE 5
#define MAX_BUFFER 100

int main() {
    int sockfd, newsockfd, clilen;
	struct sockaddr_in cli_addr, serv_addr;
    char buf[MAX_BUFFER];

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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

	listen(sockfd, MAX_QUEUE);

	printf("TCP Server Running...\n");
	int processed = 1;

	while(1) {

		clilen = sizeof(cli_addr);
		if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) < 0) {
			perror("Accept error\n");
			exit(0);
		}

        int recvlen, done = 0, sentences = 0, words = 0, characters = 0, state = 0;
		while(!done && (recvlen = recv(newsockfd, buf, MAX_BUFFER, 0)) > 0) {
			// printf("Received: %d : ", recvlen);
		    for(int i=0; i<recvlen && !(done = (buf[i]=='\0')); i++) {
                // printf("%c", buf[i]);
				characters++;
				if(buf[i] == '.') sentences++;
				if((buf[i] >= 'a' && buf[i] <= 'z' ) 
					|| (buf[i] >= 'A' && buf[i] <= 'Z') 
					|| (buf[i] >= '0' && buf[i] <= '9')) {
						if(state == 0) {
							words++;
							state = 1;
						}
				} else {
					state = 0;
				}
            }
			// printf("\n");
        }

		// sprintf(buf, "Sentences: %d\nWords: %d\nCharacters (including newlines and spaces): %d\n", sentences, words, characters);
		// send(newsockfd, buf, MAX_BUFFER, 0);
		send(newsockfd, &sentences, 4, 0);
		send(newsockfd, &words, 4, 0);
		send(newsockfd, &characters, 4, 0);
		
		close(newsockfd);

		printf("Total files processed: %d\n", processed++);
	}

    return 0;
}
