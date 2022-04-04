// Animesh Jha
// 19CS10070
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#define BUFFER_SIZE 200
const char *cmd_del = "del";
const char *cmd_get = "getbytes";

int main(int argc, char const *argv[])
{
    int N = 1;
    int sockfd = 0;
    struct sockaddr_in server_address;
    int val, err;
    char buf[BUFFER_SIZE];
    if (argc == 3 && !strcmp(cmd_del, argv[1])) // del
    {

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("Unable to create socket\n");
            exit(0);
        }

        server_address.sin_family = AF_INET;
        inet_aton("127.0.0.1", &(server_address.sin_addr));
        server_address.sin_port = htons(50000);
        if ((connect(sockfd, (struct sockaddr *)&(server_address), sizeof(server_address))) < 0)
        {
            perror("Unable to connect to server\n");
            return 0;
        }
        printf("Connection to server done\n");
        printf("%s %s\n", argv[1], argv[2]);
        send(sockfd, argv[1], strlen(argv[1]) + 1, 0);
        send(sockfd, argv[2], strlen(argv[2]) + 1, 0);
        int i = 0;
        char temp[2];
        do
        {
            int t = recv(sockfd, temp, 1, MSG_WAITALL);
            if (t <= 0)
            {
                printf("Server closed connection, delete failed\n");
                exit(1);
            }
            buf[i++] = temp[0];
        } while (temp[0] != '\0');
        printf("%s", buf);
    }
    else if (argc == 5 && !strcmp(cmd_get, argv[1])) // getbytes
    {
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("Unable to create socket\n");
            exit(0);
        }

        server_address.sin_family = AF_INET;
        inet_aton("127.0.0.1", &(server_address.sin_addr));
        server_address.sin_port = htons(50000);
        if ((connect(sockfd, (struct sockaddr *)&(server_address), sizeof(server_address))) < 0)
        {
            perror("Unable to connect to server\n");
            return 0;
        }
        printf("Connection to server done\n");
        printf("%s %s %d %d\n", argv[1], argv[2], atoi(argv[3]), atoi(argv[4]));
        for (int i = 1; i <= 4; i++)
            send(sockfd, argv[i], strlen(argv[i]) + 1, 0);

        int i = 0;
        char temp[2];
        int x = atoi(argv[3]);
        int y = atoi(argv[4]);
        for (int i = x; i <= y; i++)
        {
            int t = recv(sockfd, temp, 1, MSG_WAITALL);
            if (t <= 0)
            {
                printf("\nServer closed connection before entire receive, getbytes failed\n");
                exit(1);
            }
            printf("%c ", temp[0]);
        }
        printf("\n");
    }
    else
    {
        printf("unknown input exiting\n");
        return 0;
    }
}