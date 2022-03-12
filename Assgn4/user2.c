#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "rsocket.h"
// 19CS1 0070
const int PORT_1 = 50000 + 2 * (70);
const int PORT_2 = 50000 + 2 * (70) + 1;

int main(int argc, char *argv[])
{

    char inp[52];
    // scanf("%50[^\n]", inp);
    struct sockaddr_in p1_addr, p2_addr;
    // p1_addr.sin_family = AF_INET;
    // p1_addr.sin_port = htons(PORT_1);
    // p1_addr.sin_addr.s_addr = INADDR_ANY;

    p2_addr.sin_family = AF_INET;
    p2_addr.sin_port = htons(PORT_2);
    p2_addr.sin_addr.s_addr = INADDR_ANY;

    int sock;
    if ((sock = r_socket(AF_INET, SOCK_MRP, 0)) < 0)
    {
        perror("couldnt create socket");
        exit(1);
    }
    if ((r_bind(sock, (struct sockaddr *)&p2_addr, sizeof(p2_addr))) < 0)
    {
        perror("couldnt bind");
        exit(1);
    }
    char buf[10] = {0};
    while (1)
    {
        int len = sizeof(p1_addr);
        int pc = r_recvfrom(sock, buf, 10, 0, (struct sockaddr *)&p1_addr, &len);
        printf("Received ");
        for (int i = 0; i < pc; i++)
        {
            printf("%c", buf[i]);
        }
        struct sockaddr_in *addr_in = (struct sockaddr_in *)&p1_addr;
        char *s = inet_ntoa(addr_in->sin_addr);
        printf(" from ip address %s and port %d \n", s, htons(addr_in->sin_port));
    }
    return 0;
}
