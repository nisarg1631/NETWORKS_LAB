#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>

const int PORT = 50000;
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

const char *SUCC_CODE = "200";
const char *FAIL_CODE = "500";
const char *FAIL_COMAND_ORDER = "600";

const int CHUNK_SIZE = 200;

void stobin(char *str, uint16_t n)
{
    uint16_t i = 0;
    int idx = 0;
    for (i = 1 << 15; i > 0; i = i / 2)
    {
        if (n & i)
            str[idx++] = '1';
        else
            str[idx++] = '0';
    }
    str[16] = '\0';
}
int bintoi(char *str)
{
    return (int)strtol(str, NULL, 2);
}
typedef struct
{
    int user_done;
    int pass_done;
    int active_user;
    int sockfd;
} CLIENT_STATE;
char **parse_input(char *user_input, int *num_args)
{
    int size = 0;
    int state = 0;
    int i = 0;
    while (user_input[i] != '\0')
    {
        if (user_input[i] != ' ' && state == 0)
        {
            state = 1;
            size++;
        }
        else if (user_input[i] == ' ' && state == 1)
        {

            state = 0;
        }
        i++;
    }
    char **ret_string = (char **)calloc(sizeof(char *), size);
    char *pch;
    *num_args = size - 1;
    i = 0;
    int first = 1;
    printf("%d \n",size);
    do
    {
        if (!first)
        {
            pch = strtok(NULL, " \n");
        }
        else
        {
            pch = strtok(user_input, " \n");
            first = 0;
        }
        if (strlen(pch))
        {
            ret_string[i] = (char *)calloc(sizeof(char), sizeof(pch));
            strcpy(ret_string[i], pch);
            i++;
        }
    } while (pch && i < size);
    return ret_string;
}
void command_handler(char *recv_buffer, CLIENT_STATE *client_state, char login_info[4][25])
{
    int num_args = 0;
    char **user_cmd = parse_input(recv_buffer, &num_args);
    // printf("%s ", user_cmd[0]);
    // for (int i = 0; i < num_args; i++)
        // printf("%s ", user_cmd[i + 1]);
    if (!strcmp(user_cmd[0], CMD_user))
    {
        // printf("%d \n", strlen(SUCC_CODE) + 1);
        if (!strcmp(login_info[0], user_cmd[1]))
        {
            client_state->user_done = 1;
            client_state->active_user = 1;
            send(client_state->sockfd, SUCC_CODE, strlen(SUCC_CODE) + 1, 0);
            return;
        }
        else if (!strcmp(login_info[2], user_cmd[1]))
        {
            client_state->user_done = 1;
            client_state->active_user = 2;
            send(client_state->sockfd, SUCC_CODE, strlen(SUCC_CODE) + 1, 0);
            return;
        }
        else
        {
            send(client_state->sockfd, FAIL_CODE, strlen(FAIL_CODE) + 1, 0);
            return;
        }
    }
    if (!strcmp(user_cmd[0], CMD_PASS))
    {
        if (client_state->user_done == 0)
        {
            send(client_state->sockfd, FAIL_COMAND_ORDER, strlen(FAIL_COMAND_ORDER) + 1, 0);
            return;
        }
        int psidx = 1;
        if (client_state->active_user == 2)
        {
            psidx = 3;
        }
        if (!strcmp(user_cmd[1], login_info[psidx]))
        {
            send(client_state->sockfd, SUCC_CODE, strlen(SUCC_CODE) + 1, 0);
            // printf("Correct password\n");
            client_state->pass_done = 1;
            return;
        }
        else
        {
            send(client_state->sockfd, FAIL_CODE, strlen(FAIL_CODE) + 1, 0);
            return;
        }
    }
    if (!strcmp(user_cmd[0], CMD_QUIT))
    {
        close(client_state->sockfd);
        exit(0);
    }
    if (client_state->pass_done == 0)
    {
        send(client_state->sockfd, FAIL_COMAND_ORDER, strlen(FAIL_COMAND_ORDER) + 1, 0);
        return;
    }
    if (!strcmp(user_cmd[0], CMD_CD))
    {
        if (!chdir(user_cmd[1]))
        {
            send(client_state->sockfd, SUCC_CODE, strlen(SUCC_CODE) + 1, 0);
        }
        else
        {
            send(client_state->sockfd, FAIL_CODE, strlen(FAIL_CODE) + 1, 0);
        }
        return;
    }
    if (!strcmp(user_cmd[0], CMD_LS))
    {
        char current_directory[200];
        struct dirent *dir_files;
        DIR *dir = opendir(getcwd(current_directory, 200));
        send(client_state->sockfd, SUCC_CODE, strlen(SUCC_CODE) + 1, 0);
        char send_buff[200] = {0};
        int idx = 0;
        while (dir_files = readdir(dir))
        {
            if (dir_files->d_name[0] == '.')
                continue;
            int i = 0;
            while (dir_files->d_name[i] != '\0')
            {
                send_buff[idx++] = dir_files->d_name[i];
                i++;
                if (idx == 200)
                {
                    send(client_state->sockfd, send_buff, 200, 0);
                    idx = 0;
                }
            }
            send_buff[idx++] = '\0';
            if (idx == 200)
            {
                send(client_state->sockfd, send_buff, 200, 0);
                idx = 0;
            }
        }
        send_buff[idx++] = '\0';
        if (idx < 200)
            send_buff[idx++] = '\0';
        send(client_state->sockfd, send_buff, idx, 0);
        return;
    }
    if (!strcmp(user_cmd[0], CMD_GET))
    {
        int remote_file_fd;
        if ((remote_file_fd = open(user_cmd[1], O_RDONLY)) < 0)
        {
            send(client_state->sockfd, FAIL_CODE, strlen(FAIL_CODE) + 1, 0);
            return;
        }
        send(client_state->sockfd, SUCC_CODE, strlen(SUCC_CODE) + 1, 0);
        char send_buff[200] = {0};
        int read_len = 0;
        int L_sent = 0;
        while (1)
        {
            uint16_t read_len = read(remote_file_fd, send_buff, 200);
            char len_buff[17] = {0};
            stobin(len_buff, read_len);
            if (read_len == 200)
            {
                send(client_state->sockfd, "M", 2, 0);
                send(client_state->sockfd, len_buff, 17, 0);
                send(client_state->sockfd, send_buff, read_len, 0);
            }
            else
            {
                send(client_state->sockfd, "L", 2, 0);
                send(client_state->sockfd, len_buff, 17, 0);
                send(client_state->sockfd, send_buff, read_len, 0);
                break;
            }
        }
        close(remote_file_fd);
        printf("done sending file\n");
        return;
    }
    if (!strcmp(user_cmd[0], CMD_PUT))
    {
        int remote_fd;
        if ((remote_fd = open(user_cmd[2], O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
        {
            send(client_state->sockfd, FAIL_CODE, strlen(FAIL_CODE) + 1, 0);
            return;
        }
        send(client_state->sockfd, SUCC_CODE, strlen(SUCC_CODE) + 1, 0);
        char type_header[2] = {0};
        // uint16_t pack_sz;
        char recv_buffer[200];
        char pack_buff[17] = {0};
        while (1)
        {
            memset(recv_buffer, 0, 200);
            memset(type_header, 0, 2);
            memset(pack_buff, 0, 17);
            int r1 = 0, r2 = 0, r3 = 0;
            r1 += recv(client_state->sockfd, type_header + r1, 2, MSG_WAITALL);
            r2 += recv(client_state->sockfd, pack_buff + r2, 17, MSG_WAITALL);
            int pc_sz = bintoi(pack_buff);
            r3 += recv(client_state->sockfd, recv_buffer + r3, pc_sz, MSG_WAITALL);
            write(remote_fd, recv_buffer, pc_sz);
            if (!strcmp(type_header, "L"))
            {
                break;
            }
        }
        close(remote_fd);
        return;
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
    // int opt = 1;
    // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
    //                &opt, sizeof(opt)))
    // {
    //     perror("setsockopt");
    //     exit(EXIT_FAILURE);
    // }
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
        printf("Hi\n");
        if ((sock = accept(server_fd, (struct sockaddr *)&client_address, &client_len)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("Connection Accepted\n");
        if (fork() == 0)
        {
            // close(server_fd);

            CLIENT_STATE present_client_state;
            present_client_state.active_user = -1;
            present_client_state.pass_done = 0;
            present_client_state.user_done = 0;
            present_client_state.sockfd = sock;
            char login_info[4][25] = {0}; // 0,1 store username and pswd for u1 and 2,3 store it for u2
            char login_info_buffer[200];
            int login_fd;
            if ((login_fd = open("user.txt", O_RDONLY)) < 0)
            {
                printf("user.txt does not exist\n");
                // #warning "resolve this"
                // send(sock,  (FAIL_), sizeof( (FAIL_)), 0);
            }
            int login_info_sz = read(login_fd, login_info_buffer, 200);
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

            char recv_buffer[200] = {0};
            while (1) // handle commands
            {
                memset(recv_buffer, 0, sizeof(recv_buffer));
                int sz = recv(sock, recv_buffer, 200, 0);
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