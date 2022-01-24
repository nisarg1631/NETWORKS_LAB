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

#define CHUNK_SIZE 20
#define MAX_BUFFER 100

int main(int argc, char const *argv[]) {

    // check for command line argument
    if(argc < 2) {
        printf("Please pass text file name as command line argument.\n");
        exit(0);
    }

    // open the file
    int textfd;
    if((textfd = open(argv[1], O_RDONLY)) < 0) {
        perror("Unable to open file\n");
		exit(0);
    }

    // open a socket
    int sockfd;
	struct sockaddr_in serv_addr;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

    serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(20000);

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

    char buf[MAX_BUFFER];
    int readlen, done = 0;
    while((readlen = read(textfd, buf, CHUNK_SIZE)) > 0) {
        // for(int i=0; i<readlen; i++) printf("%c", buf[i]);
        if(readlen < CHUNK_SIZE) {
            buf[readlen] = '\0';
            done = 1;
        }
        send(sockfd, buf, CHUNK_SIZE, 0);
    }
    if(!done) {
        buf[0] = '\0';
        done = 1;
        send(sockfd, buf, CHUNK_SIZE, 0);
    }

    // recv(sockfd, buf, MAX_BUFFER, 0);
    // printf("%s", buf);
    int sentences, words, characters;
    recv(sockfd, &sentences, 4, 0);
    recv(sockfd, &words, 4, 0);
    recv(sockfd, &characters, 4, 0);
    printf("Sentences: %d\nWords: %d\nCharacters (including newlines and spaces): %d\n", sentences, words, characters);

    return 0;
}
