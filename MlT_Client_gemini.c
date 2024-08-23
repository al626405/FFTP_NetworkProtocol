// Fast File Transfer Protocol - Client Script
// Alexis Leclerc
// 08/22/2024
// Client.c Script
//Version 0.2.4

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <openssl/md5.h>

#define PORT 5475
#define CHUNK_SIZE 8192

int sock;
pthread_t *threads;
int num_threads;

void *send_commands(void *arg) {
    char buffer[CHUNK_SIZE];
    while (1) {
        printf("Enter command: ");
        fgets(buffer, CHUNK_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strlen(buffer) == 0) {
            continue;
        }

        send(sock, buffer, strlen(buffer), 0);

        // If the command is `exit`, close the connection
        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        // Receive the response from the server
        int valread = read(sock, buffer, CHUNK_SIZE - 1);
        buffer[valread] = '\0';

        // Verify checksum
        unsigned char received_md5[MD5_DIGEST_LENGTH];
        if (read(sock, received_md5, MD5_DIGEST_LENGTH) != MD5_DIGEST_LENGTH) {
            fprintf(stderr, "Failed to read checksum\n");
            break;
        }

        unsigned char calculated_md5[MD5_DIGEST_LENGTH];
        MD5(buffer, strlen(buffer), calculated_md5);

        if (memcmp(received_md5, calculated_md5, MD5_DIGEST_LENGTH) != 0) {
            fprintf(stderr, "Data integrity check failed\n");
            break;
        }

        printf("%s\n", buffer);
    }

    close(sock);
    pthread_exit(NULL);
}

int main() {
    struct sockaddr_in serv_addr;
    char buffer[CHUNK_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    // ... (rest of the code remains unchanged)

    // ... (rest of the code remains unchanged)

    // Handle authentication response
    if (strcmp(buffer, "AUTH_SUCCESS") == 0) {
        printf("Authentication successful.\n");
    } else {
        perror("Authentication failed");
        close(sock);
        return -1;
    }

    // ... (rest of the code remains unchanged)
}
