#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <unistd.h>

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
    printf("Raw socket successfully opened! Now listening for packets...\n");

    // allocate buffer to hold packet data (64kb is max possible size for IPv4 or TCP network packet)
    unsigned char buffer[65536];

    // capture loop
    while(1){
        /*
        recvfrom (int __fd, void *__restrict __buf, size_t __n, int __flags, __SOCKADDR_ARG __addr, socklen_t *__restrict __addr_len)
        raw_socket holds our file descriptor, we allocated buffer to hold packet data, size is self explanitory. no flags or extra args
        */ 
        int data_size = recvfrom(raw_socket, buffer, sizeof(buffer), 0, NULL, NULL);

        // data_size is -1 for errors, so if < 0 then error occured
        if(data_size < 0){
            // handle error
            perror("Failed to recieve.");
            return 1;
        }
        // otherwise success
        printf("Packet ripped: Size %d bytes\n", data_size);
    }

    close(raw_socket);
    return 0;
}