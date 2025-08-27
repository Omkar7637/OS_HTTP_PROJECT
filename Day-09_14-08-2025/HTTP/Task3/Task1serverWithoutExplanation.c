#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

// ---------------- recv_all_headers() ----------------
// Purpose: Read full HTTP headers using multiple recv() calls
int recv_all_headers(int client_fd, char *buffer, size_t size) {
    int total_bytes = 0;
    int recv_calls = 0;
    int bytes;

    while (1) {
        bytes = recv(client_fd, buffer + total_bytes, size - total_bytes - 1, 0);
        recv_calls++;

        if (bytes <= 0) break;  // Connection closed or error
        total_bytes += bytes;
        buffer[total_bytes] = '\0';

        // Stop once we reach end of headers (\r\n\r\n)
        if (strstr(buffer, "\r\n\r\n")) {
            break;
        }
    }

    printf("===== RAW HTTP REQUEST =====\n%s\n============================\n", buffer);
    printf("recv() was called %d times to read headers.\n", recv_calls);

    return total_bytes;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buffer[4096];

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }

    listen(server_fd, 5);
    printf("Server listening on port %d...\n", PORT);

    while (1) {
        addr_size = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);

        memset(buffer, 0, sizeof(buffer));
        recv_all_headers(client_fd, buffer, sizeof(buffer));

        // Just send a dummy response so browser doesn't hang
        char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nTask 1 Done";
        send(client_fd, response, strlen(response), 0);

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
