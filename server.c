// Fast File Transfer Protocol
// Alexis Leclerc
// 08/22/2024
// Server.C Script
//Version 0.1.9

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mariadb/mysql.h>

#define PORT 5475
#define CHUNK_SIZE 1024
#define THREAD_POOL_SIZE 4

pthread_t thread_pool[THREAD_POOL_SIZE];

void *handle_client(void *arg);
int authenticate_user(const char *username, const char *password);
void process_command(int client_fd, char *command);
void process_upload(int client_fd, char *filename);
void process_download(int client_fd, char *filename);

MYSQL *conn;

int main() {
    // Initialize MySQL connection
    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        exit(EXIT_FAILURE);
    }

    if (mysql_real_connect(conn, "localhost", "your_db_user", "your_db_password",
                           "your_database", 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&thread_pool[i], NULL, handle_client, (void *)&server_fd);
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(thread_pool[i], NULL);
    }

    mysql_close(conn);
    close(server_fd);
    return 0;
}

void *handle_client(void *arg) {
    int server_fd = *((int *)arg);
    int new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[CHUNK_SIZE];
    int valread;

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Read username and password
        valread = read(new_socket, buffer, CHUNK_SIZE);
        buffer[valread] = '\0';
        char *username = strtok(buffer, " ");
        char *password = strtok(NULL, " ");

        if (authenticate_user(username, password)) {
            send(new_socket, "AUTH_SUCCESS", strlen("AUTH_SUCCESS"), 0);
        } else {
            send(new_socket, "AUTH_FAILURE", strlen("AUTH_FAILURE"), 0);
            close(new_socket);
            continue;
        }

        // Command loop
        while ((valread = read(new_socket, buffer, CHUNK_SIZE)) > 0) {
            buffer[valread] = '\0';
            process_command(new_socket, buffer);
        }

        close(new_socket);
    }
}

int authenticate_user(const char *username, const char *password) {
    char query[256];
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM users WHERE username='%s' AND password='%s'", username, password);
    
    if (mysql_query(conn, query)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        return 0;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        return 0;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    int authenticated = atoi(row[0]);

    mysql_free_result(result);
    return authenticated > 0;
}

void process_command(int client_fd, char *command) {
    if (strncmp(command, "UPLOAD ", 7) == 0) {
        char *filename = command + 7;
        process_upload(client_fd, filename);
    } else if (strncmp(command, "DOWNLOAD ", 9) == 0) {
        char *filename = command + 9;
        process_download(client_fd, filename);
    } else {
        // Execute system command (e.g., ls, cd)
        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            send(client_fd, "ERROR: Failed to execute command\n", 33, 0);
            return;
        }
        while (fgets(command, CHUNK_SIZE, fp) != NULL) {
            send(client_fd, command, strlen(command), 0);
        }
        pclose(fp);
    }
}

void process_upload(int client_fd, char *filename) {
    char buffer[CHUNK_SIZE];
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("open");
        return;
    }

    int n;
    while ((n = recv(client_fd, buffer, CHUNK_SIZE, 0)) > 0) {
        write(fd, buffer, n);
    }
    close(fd);
}

void process_download(int client_fd, char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        close(fd);
        return;
    }

    char *file_data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return;
    }

    send(client_fd, file_data, sb.st_size, 0);

    munmap(file_data, sb.st_size);
    close(fd);
}
