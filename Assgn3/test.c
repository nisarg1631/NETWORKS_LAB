#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>
int main(){
    int fd=open("user.txt",O_RDONLY);
    int in_size=0;
    char buffer[100]={0};
    while((in_size=read(fd,buffer,100))>0)
    {
        
    }

}