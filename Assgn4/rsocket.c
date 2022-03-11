#include "rsocket.h"
pthread_mutex_t mutex_ptr_rtable, mutex_ptr_utable;
const int BUF_SIZE = 52;
utable *unack_table = NULL;
rtable *recv_table = NULL;
int last_used_msg_id = 0;
void init_utable(utable *u, int s)
{
    pthread_mutex_lock(&mutex_ptr_utable);
    u->next_to_use = 0;
    u->size = s;
    u->arr = (umsg *)calloc(s, sizeof(umsg));
    pthread_mutex_unlock(&mutex_ptr_utable);
}
int add_to_utable(utable *u, umsg *msg)
{
    pthread_mutex_lock(&mutex_ptr_utable);
    if (u->next_to_use == u->size)
    {
        pthread_mutex_unlock(&mutex_ptr_utable);
        return 0;
    }
    u->arr[u->next_to_use] = *msg;
    u->next_to_use++;
    pthread_mutex_unlock(&mutex_ptr_utable);
    return 1;
};
int remove_from_utable(utable *u, int msg_id)
{
    pthread_mutex_lock(&mutex_ptr_utable);
    for (int i = 0; i < u->next_to_use; i++)
    {
        if (u->arr[i].msg_id == msg_id)
        {
            for (int j = i; j + 1 < u->next_to_use; j++)
            {
                u->arr[j] = u->arr[j + 1];
            }
            u->next_to_use--;
            pthread_mutex_unlock(&mutex_ptr_utable);

            return 1;
        }
    }
    pthread_mutex_unlock(&mutex_ptr_utable);
    return 0;
};
void init_rtable(rtable *r, int s)
{
    pthread_mutex_lock(&mutex_ptr_rtable);
    r->next_to_use = 0;
    r->size = s;
    r->arr = (rmsg *)calloc(s, sizeof(rmsg));
    pthread_mutex_unlock(&mutex_ptr_rtable);
}
int add_to_rtable(rtable *r, rmsg *msg)
{
    pthread_mutex_lock(&mutex_ptr_rtable);

    if (r->next_to_use == r->size)
    {
        pthread_mutex_unlock(&mutex_ptr_rtable);
        return 0;
    }
    r->arr[r->next_to_use] = *msg;
    r->next_to_use++;
    pthread_mutex_unlock(&mutex_ptr_rtable);
    return 1;
}
int remove_from_rtable(rtable *r, rmsg *msg)
{
    pthread_mutex_lock(&mutex_ptr_rtable);
    for (int i = 0; i < r->next_to_use; i++)
    {
        if (r->arr[i].msg_id == msg->msg_id)
        {
            for (int j = i; j + 1 < r->next_to_use; j++)
            {
                r->arr[j] = r->arr[j + 1];
            }
            r->next_to_use--;
            pthread_mutex_unlock(&mutex_ptr_rtable);
            return 1;
        }
    }
    pthread_mutex_unlock(&mutex_ptr_rtable);
    return 0;
}
int r_socket(int domain, int type, int protocol)
{
    if (type ^ SOCK_MRP)
    {
        return -1;
    }

    srand(time(NULL));
    // create tables
    unack_table = (utable *)calloc(1, sizeof(utable));
    recv_table = (rtable *)calloc(1, sizeof(rtable));
    init_utable(unack_table, 75);
    init_rtable(recv_table, 75);

    // create mutexes
    pthread_mutex_init(&mutex_ptr_rtable, NULL);
    pthread_mutex_init(&mutex_ptr_utable, NULL);

    // create socket
    int sock = socket(domain, SOCK_DGRAM, protocol);

    // create thread r and s
    pthread_t R, S;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(R, &attr, run_thread_r, (void *)&sock);
    pthread_create(S, &attr, run_thread_s, (void *)&sock);

    return sock;
}
int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return bind(sockfd, addr, addrlen);
}
ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    char *buf_ = (char *)calloc(len + 5, sizeof(char));
    // Type of msg
    // Msg id  (assumed max 4 digits)
    buf_[0] = '0' + Data_msg;
    int n = last_used_msg_id;
    for (int i = 0; i < 4; i++)
    {
        buf_[4 - i] = (n % 10) + '0';
        n /= 10;
    }
    for (int i = 5; i < len + 5; i++)
    {
        buf_[i] = ((char *)buf)[i - 5];
    }
    ssize_t ret = sendto(sockfd, buf_, len + 5, flags, dest_addr, addrlen);
    umsg *msg = (umsg *)calloc(1, sizeof(umsg));
    // create msg here
    msg->msg_id = last_used_msg_id++;
    msg->t_sent = time(0);
    msg->dest_addr = dest_addr;
    msg->len = len;
    for (int i = 0; i < len; i++)
        msg->msg_body[i] = ((char *)buf)[i];
    add_to_utable(unack_table, msg); // this is a thread safe operation
    return ret;
}
ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    if (recv_table->next_to_use > 0) // rtable is non empty
    {
        // take the first data message in rtable
        // remove it from rtable
        // return the message
        pthread_mutex_lock(&mutex_ptr_rtable);
        for (int i = 0; i < recv_table->next_to_use; i++)
        {
            if (recv_table->arr[i].msg_type == '0' + Data_msg)
            {
                for (int j = 0; j < len && j < 52; j++)
                    ((char *)buf)[j] = recv_table->arr[i].msg_body[j];
                src_addr = recv_table->arr[i].src_addr;
                *addrlen = sizeof(struct sockaddr);
                remove_from_rtable(recv_table, recv_table->arr + i);
                pthread_mutex_unlock(&mutex_ptr_rtable);
                return strlen(buf);
            }
        }
        pthread_mutex_unlock(&mutex_ptr_rtable);
    }
    // no data msg found, so block
    struct timespec req, rem;
    req.tv_sec = 0;
    req.tv_nsec = T;
    nanosleep(&req, &rem);
    r_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}
void run_thread_r(void *param)
{
    // waits for a message to come in a recvfrom() call. When it receives a message, if it is a data message, it stores it in the
    //  received-message table, and sends an ACK message to the sender. If it is an ACK message
    //  in response to a previously sent message, it updates the unacknowledged-message table to
    //  take out the message for which the acknowledgement has arrived.
    int *sockptr = (int *)param;
    int sock = *sockptr;
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
        if (buf[0] == '0' + Data_msg)
        {
            rmsg *msg = (rmsg *)calloc(1, sizeof(rmsg));
            msg->msg_type = buf[0] - '0';
            int n = 0;
            for (int i = 1; i <= 4; i++)
            {
                n += (buf[i] - '0');
                n *= 10;
            }
            msg->msg_id = n;
            msg->src_addr = (struct sockaddr *)&src_addr;
            for (int i = 5; i < pcsz; i++)
                msg->msg_body[i - 5] = buf[i];

            add_to_rtable(recv_table, msg); // thread safe

            // create and send ack
            char buf_[5];
            buf_[0] = '0' + ACK_msg;
            int n = msg->msg_id;
            for (int i = 1; i <= 4; i++)
            {
                buf_[4 - i] = (n % 10) + '0';
                n /= 10;
            }
            sendto(sock, buf_, 5, 0, (struct sockaddr *)&src_addr, addrlen);
        }
        // if it is an ack message,
        else if (buf[0] == '0' + ACK_msg)
        {
            //  remove from utable
            int msg_id = 0;
            for (int i = 1; i <= 4; i++)
            {
                msg_id += (buf[i] - '0');
                msg_id *= 10;
            }
            remove_from_utable(unack_table, msg_id); // thread safe
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
    int *sockptr = (int *)param;
    int sock = *sockptr;
    char buf_[BUF_SIZE + 5] = {0};
    while (1)
    {
        pthread_mutex_lock(&mutex_ptr_utable);
        for (int i = 0; i < unack_table->next_to_use; i++)
        {
            if (unack_table->arr[i].t_sent + 2 * T <= time(0))
            {
                // create buffer from msg
                buf_[0] = '0' + Data_msg;
                int n = unack_table->arr[i].msg_id;
                for (int i = 0; i < 4; i++)
                {
                    buf_[4 - i] = (n % 10) + '0';
                    n /= 10;
                }
                for (int i = 5; i < unack_table->arr[i].len + 5; i++)
                    buf_[i] = ((char *)unack_table->arr[i].msg_body)[i - 5];
                // resend
                sendto(sock, buf_, unack_table->arr[i].len + 5, 0, unack_table->arr[i].dest_addr, sizeof(unack_table->arr[i].dest_addr));
                unack_table->arr[i].t_sent = time(0);
            }
        }
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
