// Fast File Transfer Protocol - Client Script
// Alexis Leclerc
// 08/23/2024
// Client.c Script
// Version 1.0.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 5475
#define CHUNK_SIZE 8192

SSL_CTX *ctx;
int sock;
pthread_t *threads;
int num_threads;

void initialize_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

SSL_CTX *create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_client_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void cleanup_openssl() {
    EVP_cleanup();
}

void send_file(SSL *ssl, const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        printf("File not found: %s\n", file_path);
        return;
    }

    // Notify server that a file is being sent
    char command[256];
    snprintf(command, sizeof(command), "send_file %s", file_path);
    SSL_write(ssl, command, strlen(command));

    // Read file and send in chunks
    char buffer[CHUNK_SIZE];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        SSL_write(ssl, buffer, bytes_read);
    }

    fclose(file);
    printf("File sent: %s\n", file_path);
}

void *send_commands(void *arg) {
    SSL *ssl = (SSL *)arg;
    char buffer[CHUNK_SIZE];
    while (1) {
        printf("Enter command: ");
        fgets(buffer, CHUNK_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove trailing newline

        if (strlen(buffer) == 0) {
            continue;
        }

        // Handle file upload command
        if (strncmp(buffer, "send_file", 9) == 0) {
            char file_path[256];
            sscanf(buffer, "send_file %s", file_path);
            send_file(ssl, file_path);
        } else {
            SSL_write(ssl, buffer, strlen(buffer));  // Send the command to the server
        }

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        int valread = SSL_read(ssl, buffer, CHUNK_SIZE - 1);  // Receive output from server
        buffer[valread] = '\0';
        printf("%s\n", buffer);  // Print the output
    }

    SSL_free(ssl);
    pthread_exit(NULL);
}

int main() {
    struct sockaddr_in serv_addr;
    char buffer[CHUNK_SIZE] = {0};

    // Initialize OpenSSL
    initialize_openssl();
    ctx = create_context();

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }

    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        close(sock);
        SSL_free(ssl);
        return -1;
    }

    // Authentication phase
    char username[50], password[50];
    printf("Username: ");
    scanf("%s", username);
    printf("Password: ");
    scanf("%s", password);

    snprintf(buffer, sizeof(buffer), "%s %s", username, password);
    SSL_write(ssl, buffer, strlen(buffer));

    int valread = SSL_read(ssl, buffer, CHUNK_SIZE);
    buffer[valread] = '\0';

    if (strcmp(buffer, "AUTH_SUCCESS") == 0) {
        printf("Authentication successful.\n");
    } else {
        printf("Authentication failed.\n");
        SSL_free(ssl);
        close(sock);
        cleanup_openssl();
        return -1;
    }

    // Command sending phase
    num_threads = 1;  // Adjust as needed for your use case
    threads = malloc(num_threads * sizeof(pthread_t));

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, send_commands, ssl);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    close(sock);
    cleanup_openssl();
    return 0;
}
