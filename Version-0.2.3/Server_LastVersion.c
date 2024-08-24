// Fast File Transfer Protocol - Server Script
// Alexis Leclerc
// 08/22/2024
// Server.c Script
//Version 0.2.3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <mysql/mysql.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <sys/socket.h>

#define PORT 5475
#define CHUNK_SIZE 8192  // Increased chunk size for better performance
#define DB_HOST "localhost"
#define DB_USER "username"
#define DB_PASS "password"
#define DB_NAME "file_transfer"

int sock;
pthread_t *threads;
int num_threads;

typedef struct {
    int client_sock;
} thread_args_t;

void *client_thread(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    int client_sock = args->client_sock;
    char buffer[CHUNK_SIZE];
    int valread;

    int flags = fcntl(client_sock, F_GETFL, 0);
    fcntl(client_sock, F_SETFL, flags | O_NONBLOCK);  // Non-blocking mode

    while (1) {
        valread = read(client_sock, buffer, CHUNK_SIZE);
        if (valread > 0) {
            buffer[valread] = '\0';
            if (strcmp(buffer, "exit") == 0) {
                break;
            }
            printf("Received: %s\n", buffer);
            send(client_sock, buffer, strlen(buffer), 0);
        }
    }

    close(client_sock);
    free(arg);  // Free the dynamically allocated memory
    pthread_exit(NULL);
}

int authenticate_user(char *username, char *password) {
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        return 0;
    }

    char query[256];
    snprintf(query, sizeof(query), "SELECT * FROM users WHERE username='%s' AND password=MD5('%s')", username, password);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        mysql_close(conn);
        return 0;
    }

    res = mysql_store_result(conn);
    int auth_success = (mysql_num_rows(res) > 0);

    mysql_free_result(res);
    mysql_close(conn);
    return auth_success;
}

int main() {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[CHUNK_SIZE] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(sock, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    // Determine number of threads
    num_threads = get_nprocs() - 1;  // Use number of processors minus 1
    threads = malloc(num_threads * sizeof(pthread_t));

    while ((client_sock = accept(sock, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
        // Authenticate user
        read(client_sock, buffer, CHUNK_SIZE);
        char username[50], password[50];
        sscanf(buffer, "%s %s", username, password);

        if (authenticate_user(username, password)) {
            send(client_sock, "AUTH_SUCCESS", strlen("AUTH_SUCCESS"), 0);
            printf("User %s authenticated.\n", username);

            // Prepare thread arguments
            thread_args_t *args = malloc(sizeof(thread_args_t));
            args->client_sock = client_sock;

            // Start a thread for each connection
            pthread_create(&threads[num_threads++ % num_threads], NULL, client_thread, args);
        } else {
            send(client_sock, "AUTH_FAILED", strlen("AUTH_FAILED"), 0);
            close(client_sock);
        }
    }

    if (client_sock < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    free(threads);
    close(sock);
    return 0;
}
