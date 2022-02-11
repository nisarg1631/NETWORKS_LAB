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
    int sock;
    int client_length;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    char user_ip[16];
    int user_port;
} client_status;

// // checks the order of client commands
// int command_order(client_status *CLIENT_STATUS)
// {
//     if (!CLIENT_STATUS->open_done && !CLIENT_STATUS->user_done && !CLIENT_STATUS->pass_done)
//         return 1;
//     if (CLIENT_STATUS->open_done && !CLIENT_STATUS->user_done && !CLIENT_STATUS->pass_done)
//         return 1;
//     if (CLIENT_STATUS->open_done && CLIENT_STATUS->user_done && !CLIENT_STATUS->pass_done)
//         return 1;
//     return 0;
// }

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
void command_handler(char **cmd_and_args, int num_args, client_status *CLIENT_STATUS)
{
    char *user_cmd;
    strcpy(user_cmd, cmd_and_args[0]);
    char *raw_cmd;
    strcpy(raw_cmd, user_cmd);
    for (int i = 1; i <= num_args; i++)
        strcat(raw_cmd, cmd_and_args[i]);
    if (!strcmp(user_cmd, CMD_OPEN))
    {
        if (CLIENT_STATUS->open_done)
        {
            printf("Connection already open\n");
            return;
        }
        if (num_args < 2)
        {
            printf("Malformed open request, format open <ip> <port>\n");
            return;
        }
        strcpy(CLIENT_STATUS->user_ip, cmd_and_args[1]);
        CLIENT_STATUS->user_port = atoi(cmd_and_args[2]);
        if (CLIENT_STATUS->user_port < MIN_PORT || CLIENT_STATUS->user_port > MAX_PORT)
        {
            printf("Port should be between 20000 and 65535\n");
            return;
        }
        CLIENT_STATUS->sock = socket(AF_INET, SOCK_STREAM, 0);
        if (CLIENT_STATUS->sock < 0)
        {
            perror("Unable to create a socket\n");
            return;
        }
        CLIENT_STATUS->server_address.sin_family = AF_INET;
        inet_aton(CLIENT_STATUS->user_ip, &(CLIENT_STATUS->server_address.sin_addr));
        CLIENT_STATUS->server_address.sin_port = htons(CLIENT_STATUS->user_port);
        if ((connect(CLIENT_STATUS->sock, (struct sockaddr *)&(CLIENT_STATUS->server_address), sizeof(CLIENT_STATUS->server_address))) < 0)
        {
            perror("Unable to connect to server\n");
            return;
        }
        printf("Connection to server done\n");
        CLIENT_STATUS->open_done = 1;
        return;
    }

    if (!CLIENT_STATUS->open_done)
    {
        printf("Please open a valid connection first\n");
        return;
    }

    if (!strcmp(user_cmd, CMD_user))
    {
        send(CLIENT_STATUS->sock, raw_cmd, strlen(raw_cmd) + 1, 0);
        char serv_out[4] = {0};
        recv(CLIENT_STATUS->sock, serv_out, 4, 0);
        if (!strcmp(serv_out, itoa(SUCC_CODE)))
        {
            CLIENT_STATUS->user_done = 1;
        }
        else
        {
            printf("Enter valid username\n");
        }
        return;
    }
    if (!strcmp(user_cmd, CMD_PASS))
    {
        if (!CLIENT_STATUS->user_done)
        {
            printf("You must first enter the username\n");
            return;
        }
        send(CLIENT_STATUS->sock, raw_cmd, strlen(raw_cmd) + 1, 0);
        char serv_out[4] = {0};
        recv(CLIENT_STATUS->sock, serv_out, 4, 0);
        if (!strcmp(serv_out, itoa(SUCC_CODE)))
        {
            CLIENT_STATUS->pass_done = 1;
            printf("log in done\n");
        }
        else if (!strcmp(serv_out, itoa(FAIL_CODE)))
        {
            printf("incorrect password\n");
            CLIENT_STATUS->user_done = 0;
        }
        else
        {
            printf("invalid command order\n");
        }
        return;
    }
    if (!strcmp(user_cmd, CMD_LCD))
    {
        if (num_args < 1)
        {
            printf("Malformed lcd cmd, format lcd <dir_name>\n");
            return;
        }
        if (chdir(cmd_and_args[1]))
        {
            printf("could not change the directory\n");
        }
        else
        {
            printf("changed directory to %s\n", getcwd(cmd_and_args[1], strlen(cmd_and_args[1]) + 1));
        }
        return;
    }
    if (!strcmp(user_cmd, CMD_QUIT))
    {
        close(CLIENT_STATUS->sock);
        exit(0);
    }
    // other commands 
}
int main()
{
    client_status CLIENT_STATUS;
    CLIENT_STATUS.open_done = 0;
    CLIENT_STATUS.pass_done = 0;
    CLIENT_STATUS.user_done = 0;
    CLIENT_STATUS.sock = 0;
    CLIENT_STATUS.client_length = 0;
    for (int i = 0; i < 16; i++)
        CLIENT_STATUS.user_ip[i] = '\0';
    CLIENT_STATUS.user_port = 0;

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
            command_handler(user_cmd, num_args, &CLIENT_STATUS);
            while (~cnt)
                user_input[cnt--] = 0;
            cnt = 0;
        }
    }
    return 0;
}