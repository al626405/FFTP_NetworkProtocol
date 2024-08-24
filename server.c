// Fast File Transfer Protocol - Client Script
// Alexis Leclerc
// 08/23/2024
// Server.c Script
//Version 0.5.0

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
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 5475
#define CHUNK_SIZE 8192
#define DB_HOST "localhost"
#define DB_USER "username"
#define DB_PASS "password"
#define DB_NAME "file_transfer"
#define MAX_CONNECTIONS 10

SSL_CTX *ctx;

int sock;
pthread_mutex_t pool_lock;
int connection_pool[MAX_CONNECTIONS];
int connection_count = 0;

typedef struct {
    int client_sock;
    SSL *ssl;
} thread_args_t;

void *client_thread(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    int client_sock = args->client_sock;
    SSL *ssl = args->ssl;
    char buffer[CHUNK_SIZE];
    int valread;

    while (1) {
        valread = SSL_read(ssl, buffer, CHUNK_SIZE);
        if (valread > 0) {
            buffer[valread] = '\0';
            if (strcmp(buffer, "exit") == 0) {
                break;
            }
            printf("Received: %s\n", buffer);
            SSL_write(ssl, buffer, strlen(buffer));
        }
    }

    SSL_free(ssl);
    close(client_sock);
    free(arg);

    pthread_mutex_lock(&pool_lock);
    connection_count--;
    pthread_mutex_unlock(&pool_lock);

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

int main() {
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Initialize OpenSSL
    initialize_openssl();
    ctx = create_context();
    configure_context(ctx);

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
    if (listen(sock, MAX_CONNECTIONS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&pool_lock, NULL);
    printf("Server is listening on port %d\n", PORT);

    while (1) {
        if (connection_count < MAX_CONNECTIONS) {
            int client_sock = accept(sock, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (client_sock < 0) {
                perror("Accept failed");
                continue;
            }

            SSL *ssl = SSL_new(ctx);
            SSL_set_fd(ssl, client_sock);

            if (SSL_accept(ssl) <= 0) {
                ERR_print_errors_fp(stderr);
                close(client_sock);
                SSL_free(ssl);
                continue;
            }

            char buffer[CHUNK_SIZE] = {0};
            SSL_read(ssl, buffer, CHUNK_SIZE);
            char username[50], password[50];
            sscanf(buffer, "%s %s", username, password);

            if (authenticate_user(username, password)) {
                SSL_write(ssl, "AUTH_SUCCESS", strlen("AUTH_SUCCESS"));

                pthread_mutex_lock(&pool_lock);
                connection_pool[connection_count++] = client_sock;
                pthread_mutex_unlock(&pool_lock);

                thread_args_t *args = malloc(sizeof(thread_args_t));
                args->client_sock = client_sock;
                args->ssl = ssl;

                pthread_t tid;
                pthread_create(&tid, NULL, client_thread, args);
            } else {
                SSL_write(ssl, "AUTH_FAILED", strlen("AUTH_FAILED"));
                SSL_free(ssl);
                close(client_sock);
            }
        }
    }

    SSL_CTX_free(ctx);
    close(sock);
    return 0;
}
