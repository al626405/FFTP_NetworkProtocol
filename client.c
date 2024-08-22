// Fast File Transfer Protocol
// Alexis Leclerc
// 08/22/2024
// Client.C Script

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5475
#define BUFFER_SIZE 8192

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char *message;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

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

    // Send username
    message = "user";
    send(sock, message, strlen(message), 0);
    read(sock, buffer, sizeof(buffer));
    printf("Server: %s", buffer);

    // Send password
    message = "password";
    send(sock, message, strlen(message), 0);
    read(sock, buffer, sizeof(buffer));
    printf("Server: %s", buffer);

    // Command: Upload or Download
    message = "UPLOAD example.txt";
    send(sock, message, strlen(message), 0);

    // Upload file
    FILE *fp = fopen("example.txt", "rb");
    if (fp == NULL) {
        perror("File not found");
        close(sock);
        return -1;
    }
    ssize_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        send(sock, buffer, bytes_read, 0);
    }
    fclose(fp);
    read(sock, buffer, sizeof(buffer));
    printf("Server: %s", buffer);

    // Close socket
    close(sock);

    return 0;
}
