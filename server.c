#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define PORT 12345
#define BUFFER_SIZE 8192 // Increased buffer size
#define MAX_EVENTS 10

int main() {
    int server_fd, epoll_fd, nfds, i;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    struct epoll_event event, events[MAX_EVENTS];
    int window_size = 1048576; // 1 MB

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Make the socket non-blocking
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    // Set TCP window size
    setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, &window_size, sizeof(window_size));
    setsockopt(server_fd, SOL_SOCKET, SO_SNDBUF, &window_size, sizeof(window_size));

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

                // Set TCP window size for new socket
                setsockopt(new_socket, SOL_SOCKET, SO_RCVBUF, &window_size, sizeof(window_size));
                setsockopt(new_socket, SOL_SOCKET, SO_SNDBUF, &window_size, sizeof(window_size));

                // Add new socket to epoll instance
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = new_socket;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &event);
            } else {
                // Handle data from clients
                int client_fd = events[i].data.fd;
                ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE);
                if (bytes_read <= 0) {
                    // Connection closed or error
                    close(client_fd);
                } else {
                    // Process data
                    printf("Received: %s\n", buffer);
                    send(client_fd, buffer, bytes_read, 0);  // Echo back
                }
            }
        }
    }

    close(server_fd);
    close(epoll_fd);

    return 0;
}
