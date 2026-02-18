#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>


#define PACKET_SIZE 64

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;

    if (len == 1)
        sum += *(unsigned char*)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int ping_host(const char *host) {
    struct addrinfo hints, *res;
    int sockfd;
    char packet[PACKET_SIZE];
    struct icmp *icmp_hdr = (struct icmp *)packet;
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

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    memset(packet, 0, sizeof(packet));
    icmp_hdr->icmp_type = ICMP_ECHO;
    icmp_hdr->icmp_code = 0;
    icmp_hdr->icmp_id = getpid();
    icmp_hdr->icmp_seq = 1;
    icmp_hdr->icmp_cksum = checksum(packet, PACKET_SIZE);

    if (sendto(sockfd, packet, PACKET_SIZE, 0,
               (struct sockaddr *)&addr, sizeof(addr)) <= 0) {
        perror("sendto");
        close(sockfd);
        return -1;
    }

    if (recv(sockfd, packet, sizeof(packet), 0) > 0) {
        printf("Reply from %s\n", host);
    } else {
        printf("No reply from %s\n", host);
    }

    close(sockfd);
    return 0;
}

int main() {
    FILE *file = fopen("naughtylist", "r");
    if (!file) {
        perror("fopen");
        return 1;
    }

    char host[256];

    while (fgets(host, sizeof(host), file)) {
        host[strcspn(host, "\r\n")] = 0;  // strip newline
        if (strlen(host) == 0)
            continue;

        printf("Pinging %s...\n", host);
        ping_host(host);
    }

    fclose(file);
    return 0;
}
