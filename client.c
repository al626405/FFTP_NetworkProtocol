#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT 12345
#define BUFFER_SIZE 8192 // Increased buffer size

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char *message = "Hello, Server!";
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    // Make the socket non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    // Send data to server
    send(sock, message, strlen(message), 0);

    // Read response from server
    ssize_t bytes_read = read(sock, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        printf("Response from server: %s\n", buffer);
    } else {
        perror("Read failed");
    }

    // Close socket
    close(sock);

    return 0;
}
