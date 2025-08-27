#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr;
    char buffer[3000];
    
    // 1. Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket failed");
        exit(1);
    }

    // 2. Bind to port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }

    // 3. Listen
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(1);
    }
    printf("HTTP Server running on port %d...\n", PORT);

    while (1) {
        socklen_t addr_len = sizeof(server_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&server_addr, &addr_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        // 4. Read request
        read(client_fd, buffer, sizeof(buffer));
        printf("Request:\n%s\n", buffer);

        // 5. Send HTTP response
        char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 46\r\n"
            "\r\n"
            "<html><body><h1>Hello from C Server!</h1></body></html>";
        
        write(client_fd, response, strlen(response));
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
