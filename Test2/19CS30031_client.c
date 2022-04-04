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

#define MAX_BUFFER 1000

int main(int argc, char const *argv[]) {

    // check for command line argument
    if(argc < 3) {
        printf("Please pass command and filename as command line argument.\n");
        exit(0);
    }

    if(!strcmp(argv[1], "getbytes")) {
        if(argc < 5) {
            printf("Please pass start and last byte as command line argument.\n");
            exit(0);
        }
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
	serv_addr.sin_port	= htons(20002);

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

    char buf[MAX_BUFFER];
    strcpy(buf, argv[1]);
    printf("%s\n%d\n", buf, strlen(buf)+1);
    send(sockfd, buf, strlen(buf)+1, 0);
    strcpy(buf, argv[2]);
    printf("%s\n%d\n", buf, strlen(buf)+1);
    send(sockfd, buf, strlen(buf)+1, 0);

    if(!strcmp(argv[1], "getbytes")) {
        int x, y;
        x = atoi(argv[3]);
        y = atoi(argv[4]);
        send(sockfd, &x, 4, 0);
        send(sockfd, &y, 4, 0);
        printf("%d %d\n", x, y);
    } else {
        
    }

    close(sockfd);
    // char buf[MAX_BUFFER];
    // int readlen, done = 0;
    // while((readlen = read(textfd, buf, CHUNK_SIZE)) > 0) {
    //     // for(int i=0; i<readlen; i++) printf("%c", buf[i]);
    //     if(readlen < CHUNK_SIZE) {
    //         buf[readlen] = '\0';
    //         done = 1;
    //     }
    //     send(sockfd, buf, CHUNK_SIZE, 0);
    // }
    // if(!done) {
    //     buf[0] = '\0';
    //     done = 1;
    //     send(sockfd, buf, CHUNK_SIZE, 0);
    // }

    // // recv(sockfd, buf, MAX_BUFFER, 0);
    // // printf("%s", buf);
    // int sentences, words, characters;
    // recv(sockfd, &sentences, 4, 0);
    // recv(sockfd, &words, 4, 0);
    // recv(sockfd, &characters, 4, 0);
    // printf("Sentences: %d\nWords: %d\nCharacters (including newlines and spaces): %d\n", sentences, words, characters);

    return 0;
}
