#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>

const int PORT = 42424;
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
typedef struct
{
    int user_done;
    int pass_done;
    int active_user;
    int sockfd;
} CLIENT_STATE;
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
void command_handler(char *recv_buffer, CLIENT_STATE *client_state, char login_info[4][25])
{
    int num_args = 0;
    char **user_cmd = parse_input(recv_buffer, &num_args);
    if (!strcmp(user_cmd[0], CMD_user))
    {
        if (!strcmp(login_info[0], user_cmd[1]))
        {
            client_state->user_done = 1;
            client_state->active_user = 0;
            send(client_state->sockfd, itoa(SUCC_CODE), sizeof(itoa(SUCC_CODE)), 0);
            return;
        }
        else if (!strcmp(login_info[2], user_cmd[1]))
        {
            client_state->user_done = 1;
            client_state->active_user = 1;
            send(client_state->sockfd, itoa(SUCC_CODE), sizeof(itoa(SUCC_CODE)), 0);
            return;
        }
        else
        {
            send(client_state->sockfd, itoa(FAIL_CODE), sizeof(itoa(FAIL_CODE)), 0);
            return;
        }
    }
    if (!strcmp(user_cmd[0], CMD_PASS))
    {
        if (client_state->user_done == 0)
        {
            send(client_state->sockfd, itoa(FAIL_COMAND_ORDER), sizeof(itoa(FAIL_COMAND_ORDER)), 0);
            return;
        }
        int psidx = 1;
        if (client_state->active_user)
        {
            psidx = 3;
        }
        if (!strcmp(user_cmd[1], login_info[psidx]))
        {

            client_state->pass_done = 1;
            send(client_state->sockfd, itoa(SUCC_CODE), sizeof(itoa(SUCC_CODE)), 0);
            return;
        }
        else
        {
            send(client_state->sockfd, itoa(FAIL_CODE), sizeof(itoa(FAIL_CODE)), 0);
            return;
        }
    }
    if (!strcmp(user_cmd[0], CMD_QUIT))
    {
        close(client_state->sockfd);
        exit(0);
    }
    if (!strcmp(user_cmd[0], CMD_CD))
    {
        if (chdir(user_cmd[1]))
        {
            send(client_state->sockfd, itoa(SUCC_CODE), sizeof(itoa(SUCC_CODE)), 0);
        }
        else
        {
            send(client_state->sockfd, itoa(FAIL_CODE), sizeof(itoa(FAIL_CODE)), 0);
        }
        return;
    }
    if (!strcmp(user_cmd[0], CMD_LS))
    {
    }
    printf("INVALD COMMAND\n");
}
int main()
{
    int server_fd, sock, client_len;
    struct sockaddr_in client_address, server_address;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed\n");
        exit(EXIT_FAILURE);
    }
    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        client_len = sizeof(client_address);
        if ((sock = accept(server_fd, (struct sockaddr *)&client_address, &client_len)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("Connection Accepted\n");
        if (!fork())
        {
            close(server_fd);

            CLIENT_STATE present_client_state;
            present_client_state.active_user = -1;
            present_client_state.pass_done = 0;
            present_client_state.user_done = 0;
            present_client_state.sockfd = sock;
            char login_info[4][25] = {0}; // 0,1 store username and pswd for u1 and 2,3 store it for u2
            char login_info_buffer[100];
            int login_fd;
            if ((login_fd = open("user.txt", O_RDONLY)) < 0)
            {
                printf("user.txt does not exist\n");
#warning "resolve this"
                // send(sock, itoa(FAIL_), sizeof(itoa(FAIL_)), 0);
            }
            int login_info_sz = read(login_fd, login_info_buffer, 100);
            int login_idx = 0;
            char *pch;
            do
            {
                if (login_idx)
                    pch = strtok(NULL, " \n\t");
                else
                    pch = strtok(login_info_buffer, " \n\t");
                strcpy(login_info[login_idx], pch);
                login_idx++;
            } while (pch && login_idx < 4);
            char recv_buffer[100] = {0};
            while (1) // handle commands
            {
                memset(recv_buffer, 0, sizeof(recv_buffer));
                int sz = rev(sock, recv_buffer, 100, 0);
                recv_buffer[sz] = '\0';
                printf("%s\n", recv_buffer);
                command_handler(recv_buffer, &present_client_state, login_info);
            }
            printf("closing socket\n");
            close(sock);
            exit(0);
        }
        close(sock);
    }
    return 0;
}