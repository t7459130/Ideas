#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#define PACKET_SIZE 64

struct icmp_echo {
    u_int8_t  type;
    u_int8_t  code;
    u_int16_t checksum;
    u_int16_t id;
    u_int16_t seq;
};

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;

    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    if (len == 1)
        sum += *(unsigned char*)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return ~sum;
}

int ping_host(const char *host) {
    struct addrinfo hints, *res;
    int sockfd;
    char packet[PACKET_SIZE];
    struct icmp_echo *icmp_hdr = (struct icmp_echo *)packet;
    struct sockaddr_in addr;
    struct timeval timeout = {1, 0};

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    if (getaddrinfo(host, NULL, &hints, &res) != 0) {
        fprintf(stderr, "Name resolution failed for %s\n", host);
        return -1;
    }

    addr = *(struct sockaddr_in *)res->ai_addr;
    freeaddrinfo(res);

    // OpenBSD: use SOCK_DGRAM for non-root ICMP
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    memset(packet, 0, sizeof(packet));
    icmp_hdr->type = 8;   // ICMP_ECHO
    icmp_hdr->code = 0;
    icmp_hdr->id = getpid() & 0xFFFF;
    icmp_hdr->seq = 1;
    icmp_hdr->checksum = checksum(packet, PACKET_SIZE);

    if (sendto(sockfd, packet, PACKET_SIZE, 0,
               (struct sockaddr *)&addr, sizeof(addr)) <= 0) {
        perror("sendto");
        close(sockfd);}
        return -1;
    }

    if (recv(sockfd, packet, sizeof(packet), 0) > 0) {
        printf("Reply from %s\n", host);
    } else {
        printf("No reply from %s\n", host);
    }

    close(sockfd);
