#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
/* For select call */
#include <sys/select.h>
/* For socket programming calls */
#include <sys/socket.h>
#include <sys/types.h>
/* For inet_ntoa(), inet_aton(), htons(), ntohs(), etc. */
#include <netinet/in.h>
#include <arpa/inet.h>
/* For gethostbyname() */
#include <netdb.h>
/* For reading/writing UDP, IP, ICMP headers */
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
/* For close() */
#include <unistd.h>
/* For gettimeofday() */
#include <sys/time.h>

#define PAYLOAD_SIZE 52
#define CLOSED_PORT 32164
#define UDP_SRC_PORT 51000
#define MAXLEN 101

/* Structure of my_packet:
 * - IP Header (24 bytes)
 * - UDP Header (8 bytes)
 * - Payload (52 bytes)
 */
struct my_packet
{
    struct iphdr iph;
    struct udphdr udph;
    char payload[PAYLOAD_SIZE];
};

/* Fills the payload with random bytes */
void generate_random_payload(char *payload)
{
    for (int i = 0; i < PAYLOAD_SIZE; i++)
    {
        payload[i] = rand() % 0xFF;
    }
}

/* Returns time difference between two timevals in milliseconds */
double diff_time(const struct timeval *tstart, const struct timeval *tend)
{
    return 1000 * (tend->tv_sec - tstart->tv_sec) + (tend->tv_usec - tstart->tv_usec) / 1000.0;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Expected more command-line arguments\n");
        exit(EXIT_FAILURE);
    }
    /* argv[1] holds the destination domain name */
    char *dest_domain_name = argv[1];

    /* Get the IP address corresponding to the given domain name */
    struct hostent *host = gethostbyname(dest_domain_name);
    if (NULL == host)
    {
        fprintf(stderr, "Could not find IP address for given domain name\n");
        exit(EXIT_FAILURE);
    }
    struct in_addr **addr_list = (struct in_addr **)host->h_addr_list;
    /* dest_addr points to the first IP address in the address list */
    struct in_addr *dest_addr = addr_list[0];

    int raw_sock1, raw_sock2;
    /* Open a raw socket to send UDP packets */
    if ((raw_sock1 = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0)
    {
        fprintf(stderr, "Socket error for raw_sock1: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Open a raw socket to receive ICMP packets */
    if ((raw_sock2 = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
    {
        fprintf(stderr, "Socket error for raw_sock2: %s\n", strerror(errno));
        close(raw_sock1);
        exit(EXIT_FAILURE);
    }

    /* Set IP_HDRINCL to 1 to indicate to the IP software that the header is already included */
    int opt = 1;
    const int *v = &opt;
    if (setsockopt(raw_sock1, IPPROTO_IP, IP_HDRINCL, v, sizeof(opt)) < 0)
    {
        fprintf(stderr, "Setsocketopt error1 for raw_sock1: %s\n", strerror(errno));
        close(raw_sock1);
        close(raw_sock2);
        exit(EXIT_FAILURE);
    }

    if (setsockopt(raw_sock1, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        fprintf(stderr, "Setsocketopt error2 for raw_sock1: %s\n", strerror(errno));
        close(raw_sock1);
        close(raw_sock2);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client;
    memset(&client, 0, sizeof(client));
    /* Set source information : Protocol Family, IP Address and Port */
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = INADDR_ANY;
    client.sin_port = htons(UDP_SRC_PORT);

    /* Bind the UDP raw socket to specified Address and Port */
    if (bind(raw_sock1, (const struct sockaddr *)&client, sizeof(client)) < 0)
    {
        fprintf(stderr, "Bind error for raw_sock1: %s\n", strerror(errno));
        close(raw_sock1);
        close(raw_sock2);
        exit(EXIT_FAILURE);
    }

    /* Bind the ICMP raw socket to specified Address and Port */
    if (bind(raw_sock2, (const struct sockaddr *)&client, sizeof(client)) < 0)
    {
        fprintf(stderr, "Bind error for raw_sock2: %s\n", strerror(errno));
        close(raw_sock1);
        close(raw_sock2);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    /* Set destination information : Protocol Family, IP Address and Port */
    server.sin_family = AF_INET;
    server.sin_addr = *dest_addr;
    server.sin_port = htons(CLOSED_PORT);

    /* Status variable to indicate that UDP packet successfully reached destination port */
    int finished = 0;
    for (int ttl = 1; ttl <= 16; ttl++)
    {
        int num_repeats;
        for (num_repeats = 1; num_repeats <= 3; num_repeats++)
        {
            /* Status variable to indicate that ICMP packet received is a valid one */
            int valid_recv = 0;

            /* Allocate space for a packet of type my_packet */
            struct my_packet *packet = (struct my_packet *)malloc(sizeof(struct my_packet));

            /* Fill random bytes in the payload */
            generate_random_payload(packet->payload);
            /* Populate the UDP header fields */
            packet->udph.source = client.sin_port;
            packet->udph.dest = server.sin_port;
            packet->udph.len = htons(sizeof(packet->udph) + PAYLOAD_SIZE);
            packet->udph.check = 0;
            /* Populate the IP header fields */
            packet->iph.version = 4;
            packet->iph.ihl = (sizeof(struct iphdr)) >> 2;
            packet->iph.tos = 0;
            packet->iph.tot_len = htons(sizeof(packet->iph) + sizeof(packet->udph) + PAYLOAD_SIZE);
            packet->iph.id = 0;
            packet->iph.frag_off = 0;
            packet->iph.ttl = ttl;
            packet->iph.protocol = IPPROTO_UDP;
            packet->iph.check = 0;
            packet->iph.saddr = client.sin_addr.s_addr;
            packet->iph.daddr = server.sin_addr.s_addr;

            /* Send the packet */
            if (sendto(raw_sock1, (char *)packet, sizeof(struct my_packet), 0, (const struct sockaddr *)&server, sizeof(server)) < 0)
            {
                fprintf(stderr, "Sendto error on raw_sock1: %s\n", strerror(errno));
                close(raw_sock1);
                close(raw_sock2);
                exit(EXIT_FAILURE);
            }
            /* Start the timer */
            struct timeval st_time;
            gettimeofday(&st_time, NULL);

            /* select call for receiving icmp header*/
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(raw_sock2, &readfds);
            struct timeval tv = {1, 0}; /* Must be outside while loop as we want to
                                         * resume timeout from where we stopped, if spurious packet received
                                         */

            while (1)
            {
                if (select(raw_sock2 + 1, &readfds, NULL, NULL, &tv) < 0)
                {
                    fprintf(stderr, "Select error: %s\n", strerror(errno));
                    close(raw_sock1);
                    close(raw_sock2);
                    exit(EXIT_FAILURE);
                }
                /* If wait on select for more than 1 sec, time out */
                if (FD_ISSET(raw_sock2, &readfds) == 0)
                {
                    break;
                }
                /* If something is received on raw_sock2 */
                else
                {
                    char buffer[MAXLEN];    /* Buffer to store the ICMP packet */
                    struct sockaddr_in src; /* Source's information to be stored here */
                    socklen_t len = sizeof(src);

                    /* Receive a ICMP packet and store in to buffer */
                    int nbytes = recvfrom(raw_sock2, buffer, MAXLEN, 0, (struct sockaddr *)&src, (socklen_t *)&len);
                    /* End the timer */
                    struct timeval en_time;
                    gettimeofday(&en_time, NULL);
                    /* Calculate response time by finding difference between start and end times */
                    double response_time = diff_time(&st_time, &en_time);
                    if (nbytes < 0)
                    {
                        fprintf(stderr, "Recvfrom error on raw_sock2: %s\n", strerror(errno));
                        close(raw_sock1);
                        close(raw_sock2);
                        exit(EXIT_FAILURE);
                    }

                    /* Extract the IP header */
                    struct iphdr *iphdr = (struct iphdr *)buffer;
                    /* If spurious packet, go back to wait on select call with remaining timeout */
                    if (iphdr->protocol != IPPROTO_ICMP)
                    {
                        continue;
                    }

                    /* Extract the ICMP header */
                    struct icmphdr *icmphdr = (struct icmphdr *)(buffer + sizeof(struct iphdr));
                    /* Check if Destination Unreachable ICMP received */
                    if (icmphdr->type == 3)
                    {
                        if (server.sin_addr.s_addr != src.sin_addr.s_addr)
                        {
                            fprintf(stderr, "IP Addresses given by source and found at destination do not match\n");
                            close(raw_sock1);
                            close(raw_sock2);
                            exit(EXIT_FAILURE);
                        }
                        else
                        {
                            printf("Hop_Count(%d) %s %0.3lfms\n", ttl, inet_ntoa(src.sin_addr), response_time);
                            valid_recv = 1;
                            finished = 1;
                            break;
                        }
                    }
                    /* Check if Time Exceeded ICMP received */
                    else if (icmphdr->type == 11)
                    {
                        printf("Hop_Count(%d) %s %0.3lfms\n", ttl, inet_ntoa(src.sin_addr), response_time);
                        valid_recv = 1;
                        break;
                    }
                    else
                    {
                        /* Do nothing, go back to wait on select call with remaining timeout */
                    }
                }
            }
            /* If we broke out of loop becuase
             * valid packet is received, stop the repeated sending with Hop_Count = ttl and break
             */
            if (valid_recv)
                break;
        }
        /* If Destination Unreachable ICMP was received, we have reached our goal so stop */
        if (finished)
            break;

        /* If failed to receive 3 times consecutively */
        if (num_repeats == 4)
        {
            printf("Hop_Count(%d) * * *\n", ttl);
        }
    }
    close(raw_sock1);
    close(raw_sock2);
    return 0;
}
