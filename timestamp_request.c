#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>

#define ICMP_TIMESTAMP 13
#define ICMP_TIMESTAMP_REPLY 14

// ICMP header checksum calculation
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned long sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }

    if (len == 1) {
        sum += *(unsigned char *)buf;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

// ICMP timestamp request creation
void create_icmp_timestamp_request(struct icmp *icmp_header, struct sockaddr_in *dest_addr) {
    memset(icmp_header, 0, sizeof(struct icmp));

    icmp_header->icmp_type = ICMP_TIMESTAMP;
    icmp_header->icmp_code = 0;
    icmp_header->icmp_id = getpid();
    icmp_header->icmp_seq = 1;

    struct timeval *tv = (struct timeval *)icmp_header->icmp_data;
    gettimeofday(tv, NULL);  // Set the timestamp in the request

    icmp_header->icmp_cksum = checksum((unsigned short *)icmp_header, sizeof(struct icmp) + sizeof(struct timeval));
}

// Function to process incoming ICMP timestamp reply
void process_icmp_timestamp_reply(int sockfd) {
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    char buffer[1024];
    int bytes_received;

    bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&from, &fromlen);
    if (bytes_received < 0) {
        perror("recvfrom");
        return;
    }

    struct ip *ip_header = (struct ip *)buffer;
    struct icmp *icmp_header = (struct icmp *)(buffer + (ip_header->ip_hl << 2));

    if (icmp_header->icmp_type == ICMP_TIMESTAMP_REPLY) {
        struct timeval *tv = (struct timeval *)icmp_header->icmp_data;
        printf("Received ICMP Timestamp Reply from %s:\n", inet_ntoa(from.sin_addr));
        printf("Timestamp: %ld.%06ld seconds\n", tv->tv_sec, tv->tv_usec);
    }
}

int main() {
    char target_ip[50];
    struct sockaddr_in dest_addr;
    struct icmp icmp_request;
    int sockfd;

    // Get user input for target IP address
    printf("Enter the IP address to send the timestamp request to: ");
    scanf("%s", target_ip);

    // Setup the destination address
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = 0;
    dest_addr.sin_addr.s_addr = inet_addr(target_ip);

    // Create a raw socket for sending ICMP requests
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    // Create and send ICMP Timestamp Request
    create_icmp_timestamp_request(&icmp_request, &dest_addr);
    if (sendto(sockfd, &icmp_request, sizeof(struct icmp) + sizeof(struct timeval), 0,
               (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("sendto");
        close(sockfd);
        exit(1);
    }

    printf("Sent ICMP Timestamp Request to %s\n", target_ip);

    // Wait and process ICMP Timestamp Reply
    process_icmp_timestamp_reply(sockfd);

    // Close the socket
    close(sockfd);
    return 0;
}
