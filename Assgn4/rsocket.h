#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <arpa/inet.h>

float drop_prob = 0.1;
const int SOCK_MRP = 42;
const long T = 2'000'000'000L;
int r_socket(int domain, int type, int protocol);
int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
int r_close(int fd);
int dropMessage(float p);

// The message tables are the simplest possible data structures because at one point no more than 50 msgs will be in them,
// So it seems to me that the overhead of complicated data structures is not worth it. something like a Binary Search Tree can be well suited for this task , however the constatns int O(logn) would be more than the time for a naive array based solution
// even though the array based solution might not be asymptotically optimal.
// Unacknowledged message table
typedef struct
{
    int msg_id;
    struct sockaddr *dest_addr;
    char msg_body[52];
    time_t t_sent;
    ssize_t len;
} umsg;
// Received message table
typedef struct
{
    int msg_id;
    int msg_type;
    struct sockaddr *src_addr;
    char msg_body[52];
} rmsg;

typedef struct
{
    // array
    int size;
    int next_to_use;
    umsg *arr;
} utable;
typedef struct
{
    // array
    int size;
    int next_to_use;
    rmsg *arr;
} rtable;
const int ACK_msg = 1;
const int Data_msg = 0;
