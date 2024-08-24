// Fast File Transfer Protocol - Server Script
// Alexis Leclerc
// 08/23/2024
// Server.c Script
// Version 1.0.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <shadow.h>
#include <pwd.h>
#include <crypt.h>
#include <errno.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 5475
#define CHUNK_SIZE 8192
#define MAX_CONNECTIONS 1

SSL_CTX *ctx;
pthread_mutex_t pool_lock;

void initialize_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

SSL_CTX *create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    SSL_CTX_set_ecdh_auto(ctx, 1);

    if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

int authenticate_user(const char *username, const char *password) {
    struct passwd *pw;
    struct spwd *sp;
    char *encrypted_pass;

    pw = getpwnam(username);
    if (pw == NULL) {
        printf("User not found: %s\n", username);
        return 0;
    }

    sp = getspnam(username);
    if (sp == NULL) {
        printf("Cannot access shadow entry for user: %s\n", username);
        return 0;
    }

    encrypted_pass = crypt(password, sp->sp_pwdp);

    if (strcmp(encrypted_pass, sp->sp_pwdp) == 0) {
        return 1;  // Authentication successful
    } else {
        return 0;  // Authentication failed
    }
}

void log_activity(const char *message) {
    FILE *logfile = fopen("server.log", "a");
    if (logfile == NULL) {
        perror("Failed to open log file");
        return;
    }
    fprintf(logfile, "%s\n", message);
    fclose(logfile);
}

void receive_file(SSL *ssl, const char *file_path) {
    FILE *file = fopen(file_path, "wb");
    if (!file) {
        perror("Failed to open file for writing");
        return;
    }

    char buffer[CHUNK_SIZE];
    int bytes_read;
    while ((bytes_read = SSL_read(ssl, buffer, CHUNK_SIZE)) > 0) {
        fwrite(buffer, 1, bytes_read, file);
    }

    fclose(file);
    printf("File received: %s\n", file_path);
}

void handle_file_upload(SSL *ssl, char *command) {
    char file_path[256];
    sscanf(command, "send_file %s", file_path);
    receive_file(ssl, file_path);
}

void handle_file_download(SSL *ssl, char *command) {
    char file_path[256];
    sscanf(command, "get_file %s", file_path);

    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        SSL_write(ssl, "FILE_NOT_FOUND", strlen("FILE_NOT_FOUND"));
        return;
    }

    char buffer[CHUNK_SIZE];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        SSL_write(ssl, buffer, bytes_read);
    }

    fclose(file);
    printf("File sent: %s\n", file_path);
}

void *client_handler(void *arg) {
    int client_sock = *((int *)arg);
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_sock);

    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(client_sock);
        return NULL;
    }

    char buffer[CHUNK_SIZE];
    int read_bytes = SSL_read(ssl, buffer, sizeof(buffer));
    if (read_bytes > 0) {
        buffer[read_bytes] = '\0';

        // Parse username and password
        char username[50], password[50];
        sscanf(buffer, "%s %s", username, password);

        if (authenticate_user(username, password)) {
            SSL_write(ssl, "AUTH_SUCCESS", strlen("AUTH_SUCCESS"));
            printf("User authenticated: %s\n", username);
            log_activity("User authenticated successfully");

            // Command execution loop
            while ((read_bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) {
                buffer[read_bytes] = '\0';
                printf("Received command: %s\n", buffer);

                if (strcmp(buffer, "exit") == 0) {
                    break;
                }

                if (strncmp(buffer, "send_file", 9) == 0) {
                    handle_file_upload(ssl, buffer);
                } else if (strncmp(buffer, "get_file", 8) == 0) {
                    handle_file_download(ssl, buffer);
                } else {
                    // Execute other commands
                    FILE *fp;
                    char command_output[CHUNK_SIZE] = {0};

                    fp = popen(buffer, "r");  // Execute the command and open the output as a file
                    if (fp == NULL) {
                        strcpy(command_output, "Failed to execute command\n");
                    } else {
                        fread(command_output, 1, sizeof(command_output) - 1, fp);  // Read command output
                        pclose(fp);
                    }

                    SSL_write(ssl, command_output, strlen(command_output));  // Send the output back to the client
                    log_activity(command_output);
                }
            }
        } else {
            SSL_write(ssl, "AUTH_FAILED", strlen("AUTH_FAILED"));
            printf("Authentication failed for user: %s\n", username);
            log_activity("Authentication failed");
        }
    }

    SSL_free(ssl);
    close(client_sock);
    return NULL;
}

int main() {
    initialize_openssl();
    ctx = create_context();
    configure_context(ctx);

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, MAX_CONNECTIONS) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&pool_lock, NULL);
    printf("Server is listening on port %d\n", PORT);

    while (1) {
        int client_sock = accept(server_sock, NULL, NULL);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_handler, &client_sock);
        pthread_detach(thread_id);  // Detach thread for independent execution
    }

    SSL_CTX_free(ctx);
    close(server_sock);
    return 0;
}
