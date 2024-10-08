// Fast File Transfer Protocol - Client
// Alexis Leclerc
// 08/22/2024
// Server.C Script
//Version 0.2.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/mman.h>

#define PORT 5475
#define CHUNK_SIZE 8192  // Increased chunk size for better performance

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[CHUNK_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // Login to the server
    char username[50], password[50];
    printf("Username: ");
    scanf("%s", username);
    printf("Password: ");
    scanf("%s", password);

    snprintf(buffer, sizeof(buffer), "%s %s", username, password);
    send(sock, buffer, strlen(buffer), 0);

    // Wait for the authentication response
    int valread = read(sock, buffer, CHUNK_SIZE);
    buffer[valread] = '\0';

    if (strcmp(buffer, "AUTH_SUCCESS") == 0) {
        printf("Authentication successful.\n");
    } else {
        printf("Authentication failed.\n");
        close(sock);
        return -1;
    }

    // Command loop
    while (1) {
        printf("Enter command: ");
        fgets(buffer, CHUNK_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline character

        if (strlen(buffer) == 0) {
            continue;  // Skip if no command is entered
        }

        send(sock, buffer, strlen(buffer), 0);

        // If the command is `exit`, close the connection
        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        // Receive the response from the server
        valread = read(sock, buffer, CHUNK_SIZE - 1);
        buffer[valread] = '\0';
        printf("%s\n", buffer);
    }

    close(sock);
    return 0;
}
