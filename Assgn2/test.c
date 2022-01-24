#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MAX_BUFFER 256

int main() {
    char buf[MAX_BUFFER];
    printf("Enter the DNS name: ");
    scanf("%s", buf);

    struct hostent *he;
    struct in_addr **addr_list;
    if((he = gethostbyname(buf)) == NULL) {
        herror("gethostbyname");
    } else if(*(addr_list = (struct in_addr **)he->h_addr_list) == NULL) {
        printf("0.0.0.0");  
    } else {
        for(int i = 0; addr_list[i] != NULL; i++) {
            printf("%s\n", inet_ntoa(*addr_list[i]));
        }
    }
    
    return 0;
}
