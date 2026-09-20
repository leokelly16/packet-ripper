#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <netinet/ip.h>

// global variable to handle signal interrupts better 
// (using signal.h's sig_atomic_t to ensure r/w to this glob var wont be interrupted by signal itself)
volatile sig_atomic_t keep_running = 1;
// function triggered on ctrl+c interrupt to trip flag
void intHandler(int dummy){
    keep_running = 0;
    printf("\nCaught Ctrl+C interrupt. Shutting down...\n");
}

int main() {
    // make a new raw socket for ethernet frames
    /* 
    extern int socket (int __domain, int __type, int __protocol);
    domain of AF_PACKET tells kernel we want low-level packet interface
    type of SOCK_RAW tells the kernel we want raw network frames including MAC headers
    protocol of htons(ETH_P_ALL) captures all ethernet frames (ie IPv4 and IPv6), ETH_P_ALL is defined in if_ether.h
    htons = host to network short 
    */
    int raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    // returned file descriptor is -1 for errors, so if raw_socket < 0 then socket was not opened
    if(raw_socket < 0 ){
        // handle error
        perror("Failed to create a socket (Make sure you run with sudo)\n");
        return 1;
    }
    // otherwise success
    printf("Raw socket successfully opened! Now listening for packets... (Ctrl+C to quit)\n");

    // allocate buffer to hold packet data (64kb is max possible size for IPv4 or TCP network packet)
    unsigned char buffer[65536];

    // handle user interrupt
    signal(SIGINT, intHandler);
    // capture loop
    while(keep_running){
        /*
        recvfrom (int __fd, void *__restrict __buf, size_t __n, int __flags, __SOCKADDR_ARG __addr, socklen_t *__restrict __addr_len)
        raw_socket holds our file descriptor, we allocated buffer to hold packet data, size is self explanitory. no flags or extra args
        */ 
        int data_size = recvfrom(raw_socket, buffer, sizeof(buffer), 0, NULL, NULL);

        // data_size is -1 for errors, so if < 0 then error occured
        if(data_size < 0){
            // handle error
            if (errno == EINTR) {
                // woken up by interrupt signal (say Ctrl+C), break the loop cleanly so we don't accidentally sniff ARP packet
                break; 
            }
            // otherwise failed to recieve packet error
            perror("Failed to recieve.");
            return 1;
        }
        // otherwise success
        printf("\nPacket ripped: Size %d bytes\n", data_size);
        
        // frame decoding (using ethhdr from if_ether.h, assigned to buffer)
        struct ethhdr *eth = (struct ethhdr *)buffer;
        // print the destination and source mac addresses by pulling the packet header from the buffer (byte by byte)
        printf("Destination MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
        eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
        printf("Source MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
        eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5]);
        // print the protocol type in hex (also pulled from packet header, 0x0800 is IPv4)
        // using ntohs (network to host short) to convert big endian (network byte order) to little endian (host byte order)
        printf("Protocol type: 0x%04x\n", ntohs(eth->h_proto));

        // cracking the IP header
        // check for IPv4 packet
        if(ntohs(eth->h_proto) == 0x0800){
            // the ip header is past the 14 bytes of ethernet header (iphdr in netinet/ip.h)
            struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct ethhdr));

            // use in.h struct to asign source and dest ip's for parsing with inet_ntoa
            struct in_addr source_ip;
            source_ip.s_addr = ip->saddr;
            struct in_addr dest_ip;
            dest_ip.s_addr = ip->daddr;

            // print ip's using inet_ntoa from inet.h (converts internet number to ascii representation)
            printf("Source IP address: %s\n", inet_ntoa(source_ip));
            printf("Destination IP address: %s\n", inet_ntoa(dest_ip));
        }
    }

    close(raw_socket);
    return 0;
}