// Animesh Jha
// 19CS10070
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#define BUFFER_SIZE 200
pthread_mutex_t print_mutex, delete_mutex;
char *cmd_del = "del";
char *del_msg_suc = "delete success\n";
char *cmd_get = "getbytes";
void *connection_handler(void *new_socket)
{
    int sock = *(int *)new_socket;
    pthread_mutex_lock(&print_mutex);
    printf("connection at socket %d sent to thread\n", sock);
    pthread_mutex_unlock(&print_mutex);
    char cmd_name[BUFFER_SIZE];
    int i = 0;
    char temp[2];
    while (recv(sock, temp, 1, MSG_WAITALL))
    {
        cmd_name[i++] = temp[0];
        if (temp[0] == '\0')
            break;
    }
    pthread_mutex_lock(&print_mutex);
    printf("Command: %s\n", cmd_name);
    pthread_mutex_unlock(&print_mutex);
    char file_name[BUFFER_SIZE];
    i = 0;
    while (recv(sock, temp, 1, MSG_WAITALL))
    {
        file_name[i++] = temp[0];
        if (temp[0] == '\0')
            break;
    }
    pthread_mutex_lock(&print_mutex);
    printf("Filename: %s\n", file_name);
    pthread_mutex_unlock(&print_mutex);
    if (!strcmp(cmd_name, cmd_del))
    {
        pthread_mutex_lock(&delete_mutex); // added to prevent a client from deleting a file while another is getting bytes from it
        if (remove(file_name) == 0)
        {
            pthread_mutex_lock(&print_mutex);
            printf("%s", del_msg_suc);
            pthread_mutex_unlock(&print_mutex);
            send(sock, del_msg_suc, strlen(del_msg_suc) + 1, 0);
        }
        pthread_mutex_unlock(&delete_mutex);
    }
    else if (!strcmp(cmd_name, cmd_get))
    {
        char x_str[BUFFER_SIZE];
        i = 0;
        while (recv(sock, temp, 1, MSG_WAITALL))
        {
            x_str[i++] = temp[0];
            if (temp[0] == '\0')
                break;
        }
        char y_str[BUFFER_SIZE];
        i = 0;
        while (recv(sock, temp, 1, MSG_WAITALL))
        {
            y_str[i++] = temp[0];
            if (temp[0] == '\0')
                break;
        }
        int x = atoi(x_str);
        int y = atoi(y_str);
        pthread_mutex_lock(&delete_mutex);
        FILE *fp = fopen(file_name, "rb");
        if (fp == NULL)
        {
            pthread_mutex_lock(&print_mutex);
            printf("cannot open input file\n");
            pthread_mutex_unlock(&print_mutex);
            pthread_mutex_unlock(&delete_mutex);
        }
        else
        {
            char c;
            int num_send = 0;
            for (i = 0; (c = getc(fp)) != EOF; i++)
            {
                if (i >= x && i <= y)
                {
                    num_send++;
                    send(sock, &c, 1, 0);
                }
                if (i > y)
                    break;
            }
            printf("%d bytes sent\n", num_send);
            if (num_send == y - x + 1)
            {
                pthread_mutex_lock(&print_mutex);
                printf("bytes %d to %d of file %s sent\n", x, y, file_name);
                pthread_mutex_unlock(&print_mutex);
            }
            fclose(fp);
            pthread_mutuex_unlock(&delete_mutex);
        }
    }
    else
    {
        pthread_mutex_lock(&print_mutex);
        printf("unknown command\n");
        pthread_mutex_unlock(&print_mutex);
    }
    close(sock);
    pthread_exit(0);
}
int main()
{
    int server_fd, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&(delete_mutex), &attr);
    pthread_mutex_init(&(print_mutex), &attr);
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(50000);
    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    int i = 0;
    while (1)
    {
        int *new_socket = (int *)malloc(sizeof(int));
        if ((*new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("Connection accepted\n");
        pthread_t child_thread;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&child_thread, &attr, connection_handler, (void *)new_socket);
    }
    return 0;
}
