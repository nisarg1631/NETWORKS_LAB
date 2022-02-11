#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main()
{
    int login_fd = open("user.txt", O_RDONLY);
    char login_info_buffer[100];
    int login_info_sz = read(login_fd, login_info_buffer, 100);
    char login_info[4][25] = {0}; // 0,1 store username and pswd for u1 and 2,3 store it for u2
    int login_idx = 0;
    char *pch;
    do
    {
        if (login_idx)
            pch = strtok(NULL, " \n\t");
        else
            pch = strtok(login_info_buffer, " \n\t");
        strcpy(login_info[login_idx], pch);
        printf("%s\n", login_info[login_idx]);
        login_idx++;
    } while (pch && login_idx < 4);
    return 0;
}