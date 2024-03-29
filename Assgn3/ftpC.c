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

const char *SUCC_CODE = "200";
const char *FAIL_CODE = "500";
const char *FAIL_COMAND_ORDER = "600";

const int MIN_PORT = 20000;
const int MAX_PORT = 65535;
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
    do
    {
        if (!first)
            pch = strtok(NULL, " \n");
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
void command_handler(char **cmd_and_args, int num_args, char *raw_cmd, client_status *CLIENT_STATUS)
{
    char *user_cmd = calloc(sizeof(char), sizeof(cmd_and_args[0]));
    strcpy(user_cmd, cmd_and_args[0]);
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
        recv(CLIENT_STATUS->sock, serv_out, 4, MSG_WAITALL);
        if (!strcmp(serv_out, SUCC_CODE))
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
        recv(CLIENT_STATUS->sock, serv_out, 4, MSG_WAITALL);
        // printf(" response code: %s \n", serv_out);
        if (!strcmp(serv_out, SUCC_CODE))
        {
            CLIENT_STATUS->pass_done = 1;
            printf("log in done\n");
        }
        else if (!strcmp(serv_out, FAIL_CODE))
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
        if (!CLIENT_STATUS->pass_done)
        {
            printf("You must first login\n");
            return;
        }
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
            char new_dir[200];
            getcwd(new_dir, 200);
            printf("changed directory to %s\n", new_dir);
        }
        return;
    }
    if (!strcmp(user_cmd, CMD_QUIT))
    {
        close(CLIENT_STATUS->sock);
        exit(0);
    }
    // other commands
    if (!strcmp(user_cmd, CMD_CD))
    {
        send(CLIENT_STATUS->sock, raw_cmd, strlen(raw_cmd) + 1, 0);
        char serv_out[4] = {0};
        recv(CLIENT_STATUS->sock, serv_out, 4, MSG_WAITALL);
        if (!strcmp(serv_out, SUCC_CODE))
        {
            printf(" cd success\n");
        }
        else if (!strcmp(serv_out, FAIL_CODE))
        {
            printf("Error in cd \n");
        }
        else
        {
            printf("invalid command order\n");
        }
        return;
    }
    if (!strcmp(user_cmd, CMD_LS))
    {

        send(CLIENT_STATUS->sock, raw_cmd, strlen(raw_cmd) + 1, 0);
        char serv_out[4] = {0};
        recv(CLIENT_STATUS->sock, serv_out, 4, MSG_WAITALL);
        if (!strcmp(serv_out, SUCC_CODE))
        {
            char file_buff[10] = {0};
            int packet_size = 0;
            int last_null = 0;
            int file_done = 0;
            while ((packet_size = recv(CLIENT_STATUS->sock, file_buff, 10, 0)) > 0)
            {
                for (int i = 0; i < packet_size; i++)
                {
                    if (file_buff[i] != '\0')
                    {
                        printf("%c", file_buff[i]);
                        last_null = 0;
                    }
                    else
                    {
                        printf("\n");
                        if (last_null)
                        {
                            file_done = 1;
                            break;
                        }
                        last_null = 1;
                    }
                }
                if (file_done)
                {
                    break;
                }
            }
            // printf("\n");
            return;
        }
        else if (!strcmp(serv_out, FAIL_CODE))
        {
            printf("Error in dir \n");
        }
        else
        {
            printf("invalid command order\n");
        }
        return;
    }

    if (!strcmp(user_cmd, CMD_GET))
    {

        send(CLIENT_STATUS->sock, raw_cmd, strlen(raw_cmd) + 1, 0);
        char serv_out[4] = {0};
        recv(CLIENT_STATUS->sock, serv_out, 4, MSG_WAITALL);
        if (!strcmp(serv_out, SUCC_CODE))
        {
            int local_fd;
            if ((local_fd = open(cmd_and_args[2], O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
            {
                printf(" no permission to create local file\n");
                close(local_fd);
                return;
            }
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
                r1 += recv(CLIENT_STATUS->sock, type_header + r1, 2, MSG_WAITALL);
                r2 += recv(CLIENT_STATUS->sock, pack_buff + r2, 17, MSG_WAITALL);
                int pc_sz = bintoi(pack_buff);
                r3 += recv(CLIENT_STATUS->sock, recv_buffer + r3, pc_sz, MSG_WAITALL);
                write(local_fd, recv_buffer, pc_sz);
                if (!strcmp(type_header, "L"))
                {
                    break;
                }
            }
            close(local_fd);
            printf("get done\n");
            return;
        }
        else if (!strcmp(serv_out, FAIL_CODE))
        {
            printf("Error in get \n");
        }
        else
        {
            printf("invalid command order\n");
        }
        // close(local_fd);
        return;
    }

    if (!strcmp(user_cmd, CMD_PUT))
    {
        int local_fd;
        if ((local_fd = open(cmd_and_args[1], O_RDONLY)) < 0)
        {
            printf("cant read local file, does not exist\n");
            close(local_fd);
            return;
        }
        send(CLIENT_STATUS->sock, raw_cmd, strlen(raw_cmd) + 1, 0);
        char serv_out[4] = {0};
        recv(CLIENT_STATUS->sock, serv_out, 4, MSG_WAITALL);
        if (!strcmp(serv_out, SUCC_CODE))
        {
            char send_buff[200] = {0};
            int read_len = 0;
            int L_sent = 0;
            while (1)
            {
                uint16_t read_len = read(local_fd, send_buff, 200);
                char len_buff[17] = {0};
                stobin(len_buff, read_len);
                if (read_len == 200)
                {
                    send(CLIENT_STATUS->sock, "M", 2, 0);
                    send(CLIENT_STATUS->sock, len_buff, 17, 0);
                    send(CLIENT_STATUS->sock, send_buff, read_len, 0);
                }
                else
                {
                    send(CLIENT_STATUS->sock, "L", 2, 0);
                    send(CLIENT_STATUS->sock, len_buff, 17, 0);
                    send(CLIENT_STATUS->sock, send_buff, read_len, 0);
                    break;
                }
            }
            printf("done sending file\n");
        }
        else if (!strcmp(serv_out, FAIL_CODE))
        {
            printf("Error in put \n");
        }
        else
        {
            printf("invalid command order\n");
        }
        close(local_fd);
        return;
    }

    if (!strcmp(user_cmd, CMD_MGET))
    {
        for (int i = 0; i < num_args; i++)
        {
            char ncmd[200] = "get ";
            strcat(ncmd, cmd_and_args[i + 1]);
            strcat(ncmd, " ");
            strcat(ncmd, cmd_and_args[i + 1]);
            printf("ncmd: %s \n", ncmd);
            send(CLIENT_STATUS->sock, ncmd, strlen(ncmd) + 1, 0);
            char serv_out[4] = {0};
            recv(CLIENT_STATUS->sock, serv_out, 4, MSG_WAITALL);
            if (!strcmp(serv_out, SUCC_CODE))
            {
                int local_fd;
                if ((local_fd = open(cmd_and_args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
                {
                    printf(" no permission to create local file\n");
                    close(local_fd);
                    return;
                }
                char type_header[2] = {0};
                char recv_buffer[200];
                char pack_buff[17] = {0};
                while (1)
                {
                    memset(recv_buffer, 0, 200);
                    memset(type_header, 0, 2);
                    memset(pack_buff, 0, 17);
                    int r1 = 0, r2 = 0, r3 = 0;
                    r1 += recv(CLIENT_STATUS->sock, type_header + r1, 2, MSG_WAITALL);
                    r2 += recv(CLIENT_STATUS->sock, pack_buff + r2, 17, MSG_WAITALL);
                    int pc_sz = bintoi(pack_buff);
                    r3 += recv(CLIENT_STATUS->sock, recv_buffer + r3, pc_sz, MSG_WAITALL);
                    write(local_fd, recv_buffer, pc_sz);
                    if (!strcmp(type_header, "L"))
                    {
                        break;
                    }
                }
                close(local_fd);
            }
            else
            {
                // close(local_fd);
                printf("Error in mget \n");
                break;
                return;
            }
        }
        printf("done with this command\n");
        return;
    }
    if (!strcmp(user_cmd, CMD_MPUT))
    {
        for (int i = 0; i < num_args; i++)
        {
            char ncmd[200] = "put ";
            strcat(ncmd, cmd_and_args[i + 1]);
            strcat(ncmd, " ");
            strcat(ncmd, cmd_and_args[i + 1]);
            int local_fd;
            if ((local_fd = open(cmd_and_args[i + 1], O_RDONLY)) < 0)
            {
                printf("cant read local file, does not exist\n");
                close(local_fd);
                return;
            }
            printf("ncmd: %s \n", ncmd);
            send(CLIENT_STATUS->sock, ncmd, strlen(ncmd) + 1, 0);
            char serv_out[4] = {0};
            recv(CLIENT_STATUS->sock, serv_out, 4, MSG_WAITALL);
            if (!strcmp(serv_out, SUCC_CODE))
            {
                char send_buff[200] = {0};
                int read_len = 0;
                int L_sent = 0;
                while (1)
                {
                    uint16_t read_len = read(local_fd, send_buff, 200);
                    char len_buff[17] = {0};
                    stobin(len_buff, read_len);
                    if (read_len == 200)
                    {
                        send(CLIENT_STATUS->sock, "M", 2, 0);
                        send(CLIENT_STATUS->sock, len_buff, 17, 0);
                        send(CLIENT_STATUS->sock, send_buff, read_len, 0);
                    }
                    else
                    {
                        send(CLIENT_STATUS->sock, "L", 2, 0);
                        send(CLIENT_STATUS->sock, len_buff, 17, 0);
                        send(CLIENT_STATUS->sock, send_buff, read_len, 0);
                        break;
                    }
                }
                printf("Done with put\n");
            }
            else
            {
                close(local_fd);
                printf("Error in mput \n");
                return;
            }
        }
        printf("done with mput\n");
        return;
    }
    printf("Enter a valid command\n");
}
int main()
{
    client_status CLIENT_STATUS;
    CLIENT_STATUS.open_done = 0;
    CLIENT_STATUS.pass_done = 0;
    CLIENT_STATUS.user_done = 0;
    CLIENT_STATUS.sock = 0;
    CLIENT_STATUS.client_length = 0;
    memset(CLIENT_STATUS.user_ip, 0, sizeof(CLIENT_STATUS.user_ip));
    CLIENT_STATUS.user_port = 0;

    char user_input[200] = {0};
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
            else if(inp==',')
            {
                user_input[cnt++]=' ';
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
            char *user_input_cpy = malloc(strlen(user_input) + 1);
            strcpy(user_input_cpy, user_input);
            int num_args = 0;
            char **user_cmd = parse_input(user_input, &num_args);
            // printf("command parsing done\n");
            // printf("%s ", user_cmd[0]);
            // for (int i = 0; i < num_args; i++)
            //     printf("%s ", user_cmd[i + 1]);

            command_handler(user_cmd, num_args, user_input_cpy, &CLIENT_STATUS);
            // printf("command handling done\n");
            while (~cnt)
                user_input[cnt--] = 0;
            cnt = 0;
        }
    }
    return 0;
}