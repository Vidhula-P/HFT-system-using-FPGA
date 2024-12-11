#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>    // For F_SETFL and O_NONBLOCK
#include <errno.h>    // For errno

// Function to check for packets
void check_for_packets(int sockfd, struct sockaddr_in* client_addr, socklen_t* addr_len) {
    char buffer[1024];
    int len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)client_addr, addr_len);
    if (len > 0) {
        buffer[len] = '\0'; // Null-terminate the received string
        printf("Received: %s\n", buffer);
    } else if (len == -1 && errno != EWOULDBLOCK && errno != EAGAIN) {
        perror("recvfrom error");
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Set the socket to non-blocking mode
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0) {
        perror("Failed to set non-blocking mode");
        close(sockfd);
        return -1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5000); // Port number
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the address
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }

    printf("Waiting for packets...\n");

    // Main loop
    while (1) {
        // Simulate VGA updates
        // Call the function to check for incoming packets
        check_for_packets(sockfd, &client_addr, &addr_len);

        // Simulate delay to prevent busy-waiting (optional)
        usleep(100000); // 100ms
    }

    close(sockfd);
    return 0;
}
