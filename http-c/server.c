#include "utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BACKLOG_SIZE 8

int main() {
    int socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };

    if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(socket_fd, BACKLOG_SIZE) < 0) {
        perror("listen");
        return 1;
    }
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        
        int connection_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (connection_fd < 0) {
            perror("accept");
            return 1;
        }

        handle_connection(connection_fd);
    }

    close(socket_fd);

    return 0;
}