// Fast File Transfer Protocol
// Alexis Leclerc
// 08/22/2024
// Server.C Script

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <mysql/mysql.h>

#define PORT 5475
#define BUFFER_SIZE 8192
#define MAX_EVENTS 10

// Database credentials
const char *DB_HOST = "localhost";
const char *DB_USER = "root";
const char *DB_PASS = "your_root_password";
const char *DB_NAME = "file_transfer";

int handle_client(int client_fd, MYSQL *conn);
int authenticate_user(MYSQL *conn, const char *username, const char *password);

int main() {
    int server_fd, epoll_fd, nfds, i;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    struct epoll_event event, events[MAX_EVENTS];
    MYSQL *conn;

    // Initialize MySQL connection
    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        exit(EXIT_FAILURE);
    }

    if (mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Make the socket non-blocking
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    // Define server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Create an epoll instance
    if ((epoll_fd = epoll_create1(0)) < 0) {
        perror("epoll_create1");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Add server socket to epoll instance
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
        perror("epoll_ctl: server_fd");
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    // Event loop
    while (1) {
        nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd) {
                // Handle new connections
                int new_socket;
                if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                    perror("accept");
                    continue;
                }
                // Make the new socket non-blocking
                fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);

                // Add new socket to epoll instance
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = new_socket;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &event);
            } else {
                // Handle data from clients
                int client_fd = events[i].data.fd;
                if (handle_client(client_fd, conn) < 0) {
                    close(client_fd);
                }
            }
        }
    }

    mysql_close(conn);
    close(server_fd);
    close(epoll_fd);

    return 0;
}

int handle_client(int client_fd, MYSQL *conn) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Authentication
    bytes_read = read(client_fd, buffer, BUFFER_SIZE);
    if (bytes_read <= 0) return -1;
    buffer[bytes_read] = '\0';

    char *username = strtok(buffer, "\n");
    send(client_fd, "Username OK\n", 12, 0);

    bytes_read = read(client_fd, buffer, BUFFER_SIZE);
    buffer[bytes_read] = '\0';
    char *password = strtok(buffer, "\n");

    if (authenticate_user(conn, username, password)) {
        send(client_fd, "Authenticated\n", 14, 0);
    } else {
        send(client_fd, "Authentication Failed\n", 23, 0);
        return -1;
    }

    // Command handling
    while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE)) > 0) {
        buffer[bytes_read] = '\0';
        if (strncmp(buffer, "UPLOAD ", 7) == 0) {
            char *filename = buffer + 7;
            FILE *fp = fopen(filename, "wb");
            if (fp == NULL) {
                send(client_fd, "File error\n", 11, 0);
                return -1;
            }

            while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE)) > 0) {
                fwrite(buffer, 1, bytes_read, fp);
            }
            fclose(fp);
            send(client_fd, "Upload complete\n", 16, 0);
        } else if (strncmp(buffer, "DOWNLOAD ", 9) == 0) {
            char *filename = buffer + 9;
            FILE *fp = fopen(filename, "rb");
            if (fp == NULL) {
                send(client_fd, "File not found\n", 15, 0);
                return -1;
            }

            while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
                send(client_fd, buffer, bytes_read, 0);
            }
            fclose(fp);
            send(client_fd, "Download complete\n", 18, 0);
        } else {
            send(client_fd, "Unknown command\n", 16, 0);
        }
    }
    return 0;
}

int authenticate_user(MYSQL *conn, const char *username, const char *password) {
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[2];
    my_bool is_null[1];
    char query[] = "SELECT password FROM users WHERE username = ?";
    char db_password[BUFFER_SIZE];

    stmt = mysql_stmt_init(conn);
    if (!stmt) {
        fprintf(stderr, "mysql_stmt_init() failed\n");
        return 0;
    }

    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        fprintf(stderr, "mysql_stmt_prepare() failed\n");
        mysql_stmt_close(stmt);
        return 0;
    }

    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char *)username;
    bind[0].buffer_length = strlen(username);

    if (mysql_stmt_bind_param(stmt, bind)) {
        fprintf(stderr, "mysql_stmt_bind_param() failed\n");
        mysql_stmt_close(stmt);
        return 0;
    }

    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = db_password;
    bind[0].buffer_length = BUFFER_SIZE;

    if (mysql_stmt_bind_result(stmt, bind)) {
        fprintf(stderr, "mysql_stmt_bind_result() failed\n");
        mysql_stmt_close(stmt);
        return 0
