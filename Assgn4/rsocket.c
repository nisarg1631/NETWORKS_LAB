#include "rsocket.h"
pthread_mutex_t mutex_ptr_rtable, mutex_ptr_utable;

// The message tables are the simplest possible data structures because at one point no more than 50 msgs will be in them,
// So it seems to me that the overhead of complicated data structures is not worth it. something like a Binary Search Tree can be well suited for this task , however the constatns int O(logn) would be more than the time for a naive array based solution 
// even though the array based solution might not be asymptotically optimal.
// Unacknowledged message table
typedef struct
{
    int msg_id;
    struct sockaddr_in dest_addr;
    char msg_body[52];
    time_t t_sent;
} umsg;
// Received message table
typedef struct
{
    int msg_id;
    int msg_type;
    struct sockaddr_in src_addr;
    char msg_body[52];
} rmsg;
umsg *utable;
rmsg *rtable;
int add_to_utable();
int add_to_rtable();
int remove_from_utable();
int remove_from_rtable();
int sock = -1;
const int BUF_SIZE = 52;
int r_socket(int domain, int type, int protocol)
{
    if (type ^ SOCK_MRP)
    {
        return -1;
    }

    srand(time(NULL));
    // create tables
    utable = (umsg *)calloc(50, sizeof(umsg));
    rtable = (rmsg *)calloc(50, sizeof(rmsg));
    // create thread r and s
    pthread_t R, S;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(R, &attr, run_thread_r, NULL);
    pthread_create(S, &attr, run_thread_s, NULL);
    sock = socket(domain, SOCK_DGRAM, protocol);
    return sock;
}
int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return bind(sockfd, addr, addrlen);
}
ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    ssize_t ret = sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    add_to_utable(buf, len, dest_addr);
}
ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    if () // rtable is non empty
    {
        pthread_mutex_lock(&mutex_ptr_rtable);
        // take the first message in rtable
        // remove it from rtable
        pthread_mutex_unlock(&mutex_ptr_rtable);
        // return the message
    }
    else
    {
        struct timespec req, rem;
        req.tv_sec = 0;
        req.tv_nsec = T;
        nanosleep(&req, &rem);
        r_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    }
}
void run_thread_r(void *param)
{
    // waits for a message to come in a recvfrom() call. When it receives a message, if it is a data message, it stores it in the
    //  received-message table, and sends an ACK message to the sender. If it is an ACK message
    //  in response to a previously sent message, it updates the unacknowledged-message table to
    //  take out the message for which the acknowledgement has arrived.
    char buf[BUF_SIZE] = {0};
    while (1)
    {
        struct sockaddr_in src_addr;
        int addrlen = sizeof(src_addr);
        int pcsz = recvfrom(sock, buf, BUF_SIZE, 0, (struct sockaddr *)&src_addr, &addrlen);
        if (dropMessage(drop_prob))
        {
            continue;
        }
        // process message
        // check if it is a data message
        if ()
        {
            pthread_mutex_lock(&mutex_ptr_rtable);
            //  add to rtable
            add_to_rtable();
            pthread_mutex_unlock(&mutex_ptr_rtable);
        }
        // if it is an ack message,
        else if ()
        {
            pthread_mutex_lock(&mutex_ptr_utable);
            //  remove from utable
            remove_from_utable();
            pthread_mutex_unlock(&mutex_ptr_utable);
        }
        else
        {
            printf("received malformed message\n");
        }
    }
}
void run_thread_s(void *param)
{
    // sleeps for some time (T), and wakes up periodically. On waking up, it scans the unacknowledged-message table to see if any of
    // the messages timeout period (set to 2T ) is over (from the difference between the time in
    // the table entry for a message and the current time). If yes, it retransmits that message and
    // resets the time in that entry of the table to the new sending time.
    while (1)
    {
        pthread_mutex_lock(&mutex_ptr_utable);
        // scan the unacknowledged-message table
        // if the time is over, resend the message
        pthread_mutex_unlock(&mutex_ptr_utable);

        struct timespec req, rem;
        req.tv_sec = 0;
        req.tv_nsec = T;
        nanosleep(&req, &rem);
    }
}
int dropMessage(float p)
{
    if (rand() / (float)RAND_MAX < p)
        return 1;
    return 0;
}
