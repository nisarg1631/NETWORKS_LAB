#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

const char* CMD_OPEN="open";
const char* CMD_user="user";
const char* CMD_PASS="pass";
const char* CMD_CD="cd";
const char* CMD_LCD="lcd";
const char* CMD_LS="dir";
const char* CMD_GET="get";
const char* CMD_PUT="put";
const char* CMD_MGET="mget";
const char* CMD_MPUT="mput";
const char* CMD_QUIT="quit";

const int SUCC_CODE=200;
const int FAIL_CODE=500;
const int FAIL_COMAND_ORDER=600;
const int MIN_PORT=20000;
const int MAX_PORT=65535;

struct CLIENT_AUTH{
    int open_done;
    int user_done;
    int pass_done;
} AUTH_VAR;

int main()
{
    AUTH_VAR.open_done=0;
    AUTH_VAR.pass_done=0;
    AUTH_VAR.user_done=0;
    
    int sock = 0, client_length = 0;
    struct sockaddr_in server_address, client_address;
    char user_input[100] = {0};

    while (1)
    {
        printf("myFTP> ");
        fflush(stdout);
        char inp;
        int cnt = 0;
        do
        {

        } while (inp != '\n');
        if (inp == '\n')
        {
            //parse input here
        }
    }
    // if((sock=socket(AF_INET,SOCK_STREAM,0))<0)
    // {
    //     printf("\n Socket creation error \n");
    //     return -1;
    // }
    // server_address.sin_family=AF_INET;
    // server_address.sin_port=htons(port);

    return 0;
}