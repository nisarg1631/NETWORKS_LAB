#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>

#define MAX_TTL 16
#define MAX_PROBES 3

double timespec_diff(struct timespec *start, struct timespec *stop) {
    double timediff_ms = 0;

    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        timediff_ms += (stop->tv_sec - start->tv_sec - 1) * 1000;
        timediff_ms += (stop->tv_nsec - start->tv_nsec + 1000000000) / (double)(1000000);
    } else {
        timediff_ms += (stop->tv_sec - start->tv_sec) * 1000;
        timediff_ms += (stop->tv_nsec - start->tv_nsec) / (double)(1000000);
    }

    return timediff_ms;
}

static unsigned short compute_ip_checksum(unsigned short *addr, unsigned int count) {
    register unsigned long sum = 0;
    while (count > 1) {
    sum += *addr++;
    count -= 2;
    }
    //if any bytes left, pad the bytes and add
    if(count > 0) {
    sum += ((*addr) & htons(0xFF00));
    }
    //Fold sum to 16 bits: add carrier to result
    while(sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    //one's complement
    sum = ~sum;
    return ((unsigned short)sum);
}

void compute_udp_checksum(struct iphdr *pIph, unsigned short *ipPayload) {
    register unsigned long sum = 0;
    struct udphdr *udphdrp = (struct udphdr *)(ipPayload);
    unsigned short udpLen = htons(udphdrp->len);
    //printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~udp len=%dn", udpLen);
    //add the pseudo header 
    //printf("add pseudo headern");
    //the source ip
    sum += (pIph->saddr >> 16) & 0xFFFF;
    sum += (pIph->saddr) & 0xFFFF;
    //the dest ip
    sum += (pIph->daddr >> 16) & 0xFFFF;
    sum += (pIph->daddr) & 0xFFFF;
    //protocol and reserved: 17
    sum += htons(IPPROTO_UDP);
    //the length
    sum += udphdrp->len;
 
    //add the IP payload
    //printf("add ip payloadn");
    //initialize checksum to 0
    udphdrp->check = 0;
    while (udpLen > 1) {
        sum += *ipPayload++;
        udpLen -= 2;
    }
    //if any bytes left, pad the bytes and add
    if(udpLen > 0) {
        //printf("+++++++++++++++padding: %dn", udpLen);
        sum += ((*ipPayload) & htons(0xFF00));
    }
    //Fold sum to 16 bits: add carrier to result
    //printf("add carriern");
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    //printf("one's complementn");
    sum = ~sum;
    //set computation result
    udphdrp->check = ((unsigned short)sum == 0x0000) ? 0xFFFF : (unsigned short)sum;
}

void print_header(char recvbuffer[]) {
    struct iphdr *recv_iphdr = (struct iphdr *)recvbuffer;
    int hlen1 = (recv_iphdr->ihl) << 2;
    struct icmphdr *recv_icmphdr = (struct icmphdr *)(recvbuffer + hlen1);
    struct iphdr *send_iphdr =  (struct iphdr *)(recvbuffer + hlen1 + 8);
    int hlen2 = (send_iphdr->ihl) << 2;
    struct udphdr *send_udphdr = (struct udphdr *)(recvbuffer + hlen1 + hlen2 + 8);

    struct in_addr origip, srcip;
    origip.s_addr = send_iphdr->daddr;
    srcip.s_addr = recv_iphdr->saddr;
    printf("ICMP type: %d\n", recv_icmphdr->type);
    // print all the above info
    struct in_addr daddr, saddr;
    daddr.s_addr = send_iphdr->daddr;
    saddr.s_addr = send_iphdr->saddr;
    printf("IP check: %d\n", send_iphdr->check);
    printf("IP daddr: %s\n", inet_ntoa(daddr));
    printf("IP frag_off: %d\n", send_iphdr->frag_off);
    printf("IP id: %d\n", send_iphdr->id);
    printf("IP ihl: %d\n", send_iphdr->ihl);
    printf("IP protocol: %d\n", send_iphdr->protocol);
    printf("IP saddr: %s\n", inet_ntoa(saddr));
    printf("IP tos: %d\n", send_iphdr->tos);
    printf("IP tot_len: %d\n", ntohs(send_iphdr->tot_len));
    printf("IP ttl: %d\n", send_iphdr->ttl);
    printf("IP version: %d\n", send_iphdr->version);
    // printf("IP orig: %s\n", inet_ntoa(origip));
    // printf("IP src1: %s\n", inet_ntoa(srcip));
    // printf("IP src2: %s\n", inet_ntoa(src.sin_addr));
    // printf("IP check: %d\n", send_iphdr->check);
    // printf("UDP check: %d\n", send_udphdr->check);
}

signed main(int argc, char const *argv[]) {
    if(argc < 2) {
        printf("Please enter domain name as command line argument.\n");
        exit(0);
    }

    struct hostent *he;
    struct in_addr **addr_list;
    if((he = gethostbyname(argv[1])) == NULL || *(addr_list = (struct in_addr **)he->h_addr_list) == NULL) {
        printf("DNS name couldn't be resolved.\n");
        exit(0);
    }

    struct in_addr *ip_address = addr_list[0], *self_ip_address = NULL;
    printf("IP: %s\n", inet_ntoa(*ip_address));
    struct sockaddr_in sin, din;

    struct ifaddrs *ifaddr;
    if(getifaddrs(&ifaddr) < 0) {
        perror("Failed to get network interfaces.\n");
        exit(0);
    }
    
    for (struct ifaddrs *it = ifaddr; it != NULL; it = it->ifa_next) {
        if(it->ifa_addr != NULL && it->ifa_addr->sa_family == AF_INET && (it->ifa_flags & IFF_RUNNING) && !(it->ifa_flags & IFF_LOOPBACK)) {
            self_ip_address = (struct in_addr *)malloc(sizeof(struct in_addr));
            *self_ip_address = ((struct sockaddr_in *)(it->ifa_addr))->sin_addr;
            break;
        }
    }

    din.sin_family = AF_INET;
    din.sin_port = htons(32164);
    din.sin_addr = *ip_address;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(20002);
    sin.sin_addr.s_addr = (self_ip_address == NULL) ? INADDR_ANY : self_ip_address->s_addr;

    int sock_udp;
    if((sock_udp = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0) {
        perror("Cannot create UDP raw socket: ");
        exit(0);
    }

    const int on = 1;
    if(setsockopt(sock_udp, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
        perror("Cannot set UDP raw socket option: ");
        close(sock_udp);
        exit(0);
    }

    int sock_icmp;
    if((sock_icmp = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        perror("Cannot create ICMP raw socket: ");
        close(sock_udp);
        exit(0);
    }

    int done = 0;
    for(int ttl = 1; ttl <= MAX_TTL; ttl++) {
        int probe_success = 0;
        for(int probe = 0; probe < MAX_PROBES; probe++) {
            char buf[1024];
            struct iphdr *ip_header = (struct iphdr *)buf;
            struct udphdr *udp_header = (struct udphdr *)(buf + sizeof(struct iphdr));
            char *udp_payload = (char *)(buf + sizeof(struct iphdr) + sizeof(struct udphdr));

            for(int i = 0; i < 52; i++) {
                udp_payload[i] = (char)(rand() % 256);
            }

            // FILL IP HEADER
            ip_header->check = (uint16_t)0;
            ip_header->daddr = din.sin_addr.s_addr;
            ip_header->frag_off = (uint16_t)0;
            ip_header->id = htons((uint16_t)54321);
            ip_header->ihl = 5;
            ip_header->protocol = IPPROTO_UDP;
            ip_header->saddr = sin.sin_addr.s_addr;
            ip_header->tos = 0;
            ip_header->tot_len = htons((uint16_t)(sizeof(struct iphdr) + sizeof(struct udphdr) + 52));
            ip_header->ttl = ttl;
            ip_header->version = 4;
            ip_header->check = compute_ip_checksum((unsigned short *)ip_header, sizeof(struct iphdr));

            // FILL UDP HEADER
            udp_header->check = 0;
            udp_header->dest = din.sin_port;
            udp_header->source = sin.sin_port;
            udp_header->len = htons((uint16_t)(sizeof(struct udphdr) + 52));
            // compute_udp_checksum(ip_header, (unsigned short *)(buf + sizeof(struct iphdr)));

            if(sendto(sock_udp, buf, sizeof(struct iphdr) + sizeof(struct udphdr) + 52, 0, (const struct sockaddr *)&din, sizeof(din)) < 0) {
                perror("Error in sendto: ");
                close(sock_udp);
                close(sock_icmp);
                exit(0);
            }

            struct timespec st, en;
            clock_gettime(CLOCK_MONOTONIC, &st);

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sock_icmp, &readfds);
            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            while(1) {
                if(select(sock_icmp + 1, &readfds, NULL, NULL, &tv) < 0) {
                    perror("Error in select: ");
                    close(sock_udp);
                    close(sock_icmp);
                    exit(0);
                }
                if(!FD_ISSET(sock_icmp, &readfds))
                    break;
                char recvbuffer[1024];
                int recvlen;
                struct sockaddr_in src;
                socklen_t srclen = sizeof(src);
                if((recvlen = recvfrom(sock_icmp, recvbuffer, 1024, 0, (struct sockaddr *)&src, &srclen)) < 0) {
                    perror("Error in recvfrom: ");
                    close(sock_udp);
                    close(sock_icmp);
                    exit(0);
                }
                clock_gettime(CLOCK_MONOTONIC, &en);
                double timediff_ms = timespec_diff(&st, &en);
                // print_header(recvbuffer);

                struct iphdr *recv_iphdr = (struct iphdr *)recvbuffer;
                int hlen1 = (recv_iphdr->ihl) << 2;
                struct icmphdr *recv_icmphdr = (struct icmphdr *)(recvbuffer + hlen1);
                struct iphdr *send_iphdr =  (struct iphdr *)(recvbuffer + hlen1 + 8);
                int hlen2 = (send_iphdr->ihl) << 2;
                struct udphdr *send_udphdr = (struct udphdr *)(recvbuffer + hlen1 + hlen2 + 8);

                if(recv_iphdr->protocol == IPPROTO_ICMP && send_iphdr->daddr == din.sin_addr.s_addr) {
                    struct in_addr src_addr;
                    src_addr.s_addr = recv_iphdr->saddr;
                    if(recv_icmphdr->type == ICMP_TIME_EXCEEDED) {
                        printf("Hop_Count(%d) %s %.2lfms\n", ttl, inet_ntoa(src_addr), timediff_ms);
                        probe_success = 1;
                        break;
                    }
                    if(recv_icmphdr->type == ICMP_DEST_UNREACH && recv_iphdr->saddr == din.sin_addr.s_addr) {
                        printf("Hop_Count(%d) %s %.2lfms\n", ttl, inet_ntoa(src_addr), timediff_ms);
                        probe_success = 1;
                        done = 1;
                        break;
                    }
                }
            }

            if(probe_success)
                break;
        }

        if(!probe_success)
            printf("Hop_Count(%d) * * * *\n", ttl);

        if(done)
            break;
    }

    close(sock_udp);
    close(sock_icmp);
    return 0;
}
