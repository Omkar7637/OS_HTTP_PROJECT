#include <stdio.h>      // For printf, perror
#include <stdlib.h>     // For exit()
#include <string.h>     // For memset, strstr, strcmp, strlen
#include <unistd.h>     // For close()
#include <arpa/inet.h>  // For inet_ntoa, htons, sockaddr_in
#include <sys/socket.h> // For socket(), bind(), listen(), accept(), recv(), send()
#include <netinet/in.h> // For sockaddr_in

#define PORT 8080  // Define the server port number

// ============================================================
// Function: recv_all_headers()
// Reads all HTTP headers from the client until "\r\n\r\n"
// Returns total bytes received and prints how many recv() calls.
// ============================================================
int recv_all_headers(int client_fd, char *buffer, size_t size) {
    int total_bytes = 0;   // Total number of bytes read so far
    int recv_calls = 0;    // Number of times recv() was called
    int bytes;             // Bytes read in a single recv() call

    while (1) {
        // Receive data from the client
        bytes = recv(client_fd, buffer + total_bytes, size - total_bytes - 1, 0);
        recv_calls++;

        // If recv() failed or client closed connection
        if (bytes <= 0) break;

        total_bytes += bytes;
        buffer[total_bytes] = '\0';  // Null terminate to treat as string

        // Check if we reached end of HTTP headers
        if (strstr(buffer, "\r\n\r\n")) {
            break; 
        }
    }

    // Print raw request received
    printf("===== RAW HTTP REQUEST =====\n%s\n============================\n", buffer);
    printf("recv() was called %d times to read headers.\n", recv_calls);

    return total_bytes;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buffer[4096];  // Buffer to store client request

    // ---------------- Create socket ----------------
    server_fd = socket(AF_INET, SOCK_STREAM, 0);  // TCP socket
    if (server_fd < 0) {
        perror("Socket failed");
        exit(1);
    }

    // Reuse port immediately after server restart
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Configure server address structure
    server_addr.sin_family = AF_INET;              // IPv4
    server_addr.sin_port = htons(PORT);            // Convert port to network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY;      // Accept connections from any interface

    // ---------------- Bind socket ----------------
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }

    // ---------------- Listen ----------------
    listen(server_fd, 5);  // Max 5 pending connections
    printf("Server listening on port %d...\n", PORT);

    // ---------------- Accept Loop ----------------
    while (1) {
        addr_size = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);

        // Clear buffer before each request
        memset(buffer, 0, sizeof(buffer));

        // Read HTTP request headers
        recv_all_headers(client_fd, buffer, sizeof(buffer));

        // ---------------- Task 2: Extract Method, Path, Version ----------------
        char method[16], path[256], version[16];

        // Parse the first line of request: "METHOD PATH VERSION"
        if (sscanf(buffer, "%15s %255s %15s", method, path, version) == 3) {
            printf("Method: %s\n", method);
            printf("Path: %s\n", path);
            printf("Version: %s\n", version);

            // Only allow GET requests, reject others
            if (strcmp(method, "GET") != 0) {
                char *resp_405 =
                    "HTTP/1.1 405 Method Not Allowed\r\n"
                    "Content-Type: text/html\r\n\r\n"
                    "<h1>405 - Method Not Allowed</h1>";
                send(client_fd, resp_405, strlen(resp_405), 0);
                close(client_fd);
                continue;  // Skip rest of loop
            }
        } else {
            // Invalid HTTP request line
            char *resp_400 =
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Type: text/html\r\n\r\n"
                "<h1>400 - Bad Request</h1>";
            send(client_fd, resp_400, strlen(resp_400), 0);
            close(client_fd);
            continue;  // Skip rest of loop
        }

        // ---------------- Send Response ----------------
        // For now, just send a dummy success response
        char *resp_ok =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<h1>Task 2 Done</h1>";
        send(client_fd, resp_ok, strlen(resp_ok), 0);

        // Close client connection after response
        close(client_fd);
    }

    // Close server socket (though in infinite loop, this won’t run normally)
    close(server_fd);
    return 0;
}
