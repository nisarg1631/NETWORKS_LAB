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

const char *PROMPT_START = "myFTP> ";
const char *CMD_OPEN = "open";
const char *CMD_user = "user";
const char *CMD_PASS = "pass";
const char *CMD_CD = "cd";
const char *CMD_LCD = "lcd";
const char *CMD_LS = "dir";
const char *CMD_GET = "get";
const char *CMD_PUT = "put";
const char *CMD_MGET = "mget";
const char *CMD_MPUT = "mput";
const char *CMD_QUIT = "quit";

const int SUCC_CODE = 200;
const int FAIL_CODE = 500;
const int FAIL_COMAND_ORDER = 600;
const int MIN_PORT = 20000;
const int MAX_PORT = 65535;

typedef struct
{
    int open_done;
    int user_done;
    int pass_done;
} client_auth_status;

// checks the order of client commands
int command_order(client_auth_status *CLIENT_STATUS)
{
    if (!CLIENT_STATUS->open_done && !CLIENT_STATUS->user_done && !CLIENT_STATUS->pass_done)
        return 1;
    if (CLIENT_STATUS->open_done && !CLIENT_STATUS->user_done && !CLIENT_STATUS->pass_done)
        return 1;
    if (CLIENT_STATUS->open_done && CLIENT_STATUS->user_done && !CLIENT_STATUS->pass_done)
        return 1;
    return 0;
}

// parses the input and returns a list with the command as the first element and its corresponding arguments after it
char **parse_input(char *user_input, int *num_args)
{
    int size = 1;
    int i = 0;
    while (user_input[i] != '\0')
        size += (user_input[i++] == ' ');
    char **ret_string = (char **)calloc(sizeof(char *), size);
    char *pch;
    *num_args = size - 1;
    i = 0;
    do
    {
        if (i)
            pch = strtok(NULL, " ");
        else
            pch = strtok(user_input, " ");
        ret_string[i] = (char *)calloc(sizeof(char), sizeof(pch));
        strcpy(ret_string[i], pch);
        i++;
    } while (pch && i < size);
    return ret_string;
}
void command_handler(char **cmd_and_args, client_auth_status *CLIENT_STATUS)
{
    return;
}
int main()
{
    client_auth_status CLIENT_STATUS;
    CLIENT_STATUS.open_done = 0;
    CLIENT_STATUS.pass_done = 0;
    CLIENT_STATUS.user_done = 0;
    int sock = 0, client_length = 0;
    struct sockaddr_in server_address, client_address;
    char user_input[100] = {0};
    while (1)
    {
        printf("%s", PROMPT_START);
        fflush(stdout);
        char inp;
        int cnt = 0;
        do
        {
            inp = getchar();
            if (inp == '\n' || inp == 18)
            {
                user_input[cnt] = '\0';
            }
            else
            {
                user_input[cnt++] = inp;
            }
        } while (inp != '\n');
        if (inp == '\n')
        {
            while (cnt && user_input[cnt - 1] == ' ')
            {
                user_input[cnt - 1] = '\0';
                cnt--;
            }
            int num_args = 0;
            char **user_cmd = parse_input(user_input, &num_args);
            command_handler(user_cmd, &CLIENT_STATUS);
            while (~cnt)
                user_input[cnt--] = 0;
            cnt = 0;
        }
    }
    return 0;
}