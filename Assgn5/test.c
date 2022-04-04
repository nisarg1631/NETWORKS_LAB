#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

#define MAX_TTL 16
#define MAX_PROBES 3

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

struct pseudo_header
{
	u_int32_t source_address;
	u_int32_t dest_address;
	u_int8_t placeholder;
	u_int8_t protocol;
	u_int16_t udp_length;
};

/*
	Generic checksum calculation function
*/
unsigned short csum(unsigned short *ptr,int nbytes) 
{
	register long sum;
	unsigned short oddbyte;
	register short answer;

	sum=0;
	while(nbytes>1) {
		sum+=*ptr++;
		nbytes-=2;
	}
	if(nbytes==1) {
		oddbyte=0;
		*((unsigned char *)&oddbyte)=*(unsigned char *)ptr;
		sum+=oddbyte;
	}

	sum = (sum>>16)+(sum & 0xffff);
	sum = sum + (sum>>16);
	answer=(short)~sum;
	
	return(answer);
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

    struct in_addr *ip_address = addr_list[0];
    printf("IP: %s\n", inet_ntoa(*ip_address));
    char buf[1024];
    struct sockaddr_in sin, din;

    for(int i = 0; i < 54; i++) {
        buf[i] = (char)(i+1);
    }

    din.sin_family = AF_INET;
    din.sin_port = htons(32164);
    din.sin_addr = *ip_address;

    sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(20002);

    int sock_udp;
    if((sock_udp = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Cannot create UDP raw socket: ");
        exit(0);
    }

    if(bind(sock_udp, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}
    const unsigned short ttl = (unsigned short)atoi(argv[2]); /* max = 255 */
    if(setsockopt(sock_udp, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("Unable to set sock opt: ");
        exit(0);
    }

    int sock_icmp;
    if((sock_icmp = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        perror("Cannot create ICMP raw socket: ");
        close(sock_udp);
        exit(0);
    }

    if(sendto(sock_udp, buf, 54, 0, (const struct sockaddr *)&din, sizeof(din)) < 0) {
        perror("Error in sendto: ");
        close(sock_udp);
        close(sock_icmp);
        exit(0);
    }

    printf("Data sent!\n");

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

    printf("Data received!\n");
    fflush(stdout);

    struct iphdr *recv_iphdr = (struct iphdr *)recvbuffer;
    int hlen1 = (recv_iphdr->ihl) << 2;
    struct icmphdr *recv_icmphdr = (struct icmphdr *)(recvbuffer + hlen1);
    struct iphdr *send_iphdr =  (struct iphdr *)(recvbuffer + hlen1 + 8);
    int hlen2 = (send_iphdr->ihl) << 2;
    struct udphdr *send_udphdr = (struct udphdr *)(recvbuffer + hlen1 + hlen2 + 8);
    if(recv_iphdr->protocol != IPPROTO_ICMP) {
        printf("Not an ICMP packet. :(\n");
        close(sock_udp);
        close(sock_icmp);
        exit(0);
    }

    struct in_addr origip, srcip;
    origip.s_addr = send_iphdr->daddr;
    srcip.s_addr = recv_iphdr->saddr;
    printf("ICMP type: %d\n", recv_icmphdr->type);
    // FILL IP HEADER
    // ip_header->check = 0; // TODO: recalculate this
    // ip_header->daddr = din.sin_addr.s_addr;
    // ip_header->frag_off = 0;
    // ip_header->id = htons((uint16_t)54321); // TODO: try with 1
    // ip_header->ihl = 5;
    // ip_header->protocol = IPPROTO_UDP;
    // ip_header->saddr = sin.sin_addr.s_addr; // TODO: check this
    // ip_header->tos = 0;
    // ip_header->tot_len = htons((uint16_t)(sizeof(struct iphdr) + sizeof(struct udphdr) + 52));
    // ip_header->ttl = 5;
    // ip_header->version = 4;
    // ip_header->check = compute_ip_checksum((unsigned short *)ip_header, sizeof(struct iphdr));
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

    printf("--------\n");
    for(int i=0; i<8; i++) {
        if((send_iphdr->ttl) & (1<<i)) {
            printf("1");
        } else {
            printf("0");
        }
    }
    printf("\n");

    // struct in_addr origip, srcip;
    // origip.s_addr = send_iphdr->daddr;
    // srcip.s_addr = recv_iphdr->saddr;
    // printf("ICMP type: %d\n", recv_icmphdr->type);
    // printf("IP check: %d\n", send_iphdr->check);
    // printf("IP daddr: %d\n", send_iphdr->daddr);
    // printf("IP frag_off: %d\n", send_iphdr->frag_off);
    // printf("IP id: %d\n", send_iphdr->id);
    // printf("IP ihl: %d\n", send_iphdr->ihl);
    // printf("IP protocol: %d\n", send_iphdr->protocol);
    // printf("IP saddr: %d\n", send_iphdr->saddr);
    // printf("IP tos: %d\n", send_iphdr->tos);
    // printf("IP tot_len: %d\n", send_iphdr->tot_len);
    // printf("IP ttl: %d\n", send_iphdr->ttl);
    // printf("IP version: %d\n", send_iphdr->version);
    // printf("IP orig: %s\n", inet_ntoa(origip));
    // printf("IP src1: %s\n", inet_ntoa(srcip));
    // printf("IP src2: %s\n", inet_ntoa(src.sin_addr));
    // printf("UDP check: %d\n", send_udphdr->check);

    close(sock_udp);
    close(sock_icmp);
    return 0;
}
