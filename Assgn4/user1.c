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
    printf("Enter the string to be sent: \n");
    scanf("%50[^\n]", inp);
    struct sockaddr_in p1_addr, p2_addr;
    p1_addr.sin_family = AF_INET;
    p1_addr.sin_port = htons(PORT_1);
    p1_addr.sin_addr.s_addr = INADDR_ANY;

    p2_addr.sin_family = AF_INET;
    p2_addr.sin_port = htons(PORT_2);
    p2_addr.sin_addr.s_addr = INADDR_ANY;

    int sock;
    if ((sock = r_socket(AF_INET, SOCK_MRP, 0)) < 0)
    {
        perror("couldnt create socket");
        exit(1);
    }
    if ((r_bind(sock, (struct sockaddr *)&p1_addr, sizeof(p1_addr))) < 0)
    {
        perror("couldnt bind");
        exit(1);
    }
    int i = 0;
    while (i < 50 && inp[i] != '\0')
    {
        r_sendto(sock, &inp[i], 1, 0, (struct sockaddr *)&p2_addr, sizeof(p2_addr));
        i++;
    }
    while (1)
        ;
    return 0;
}
