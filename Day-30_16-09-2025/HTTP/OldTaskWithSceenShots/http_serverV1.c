#include <stdio.h>              // For printf, perror
#include <stdlib.h>             // For exit, EXIT_FAILURE
#include <string.h>             // For memset, strlen, sprintf
#include <unistd.h>             // For close, read, write
#include <arpa/inet.h>          // For inet_ntop, htons, INET_ADDRSTRLEN
#include <signal.h>             // For signal handling (SIGINT)
#include <sys/socket.h>         // For socket, bind, listen, accept
#include <netinet/in.h>         // For sockaddr_in

// Global server socket descriptor so the signal handler can close it
int server_fd; 

// Function to handle Ctrl+C (SIGINT) and cleanly shut down the server
void handle_sigint(int sig) {
    printf("\nShutting down server...\n");
    close(server_fd); // Release the port before exiting
    exit(0);
}

int main() {
    int client_fd;                               // File descriptor for accepted client connection
    struct sockaddr_in server_addr, client_addr; // Structures to hold server and client address info
    socklen_t addr_size;                         // Size of client address structure
    char buffer[1024];                           // Buffer for incoming data

    // Register our signal handler for Ctrl+C
    signal(SIGINT, handle_sigint);

    // 1. Create a TCP socket
    // AF_INET      = IPv4
    // SOCK_STREAM  = TCP (connection-oriented)
    // Protocol 0   = Use default for SOCK_STREAM (TCP)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // 2. Set SO_REUSEADDR to reuse the port immediately after closing
    // This avoids the "Address already in use" error during quick restarts
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // 3. Configure the server address
    server_addr.sin_family = AF_INET;         // IPv4
    server_addr.sin_port = htons(8080);       // Port 8080 in network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all available network interfaces
    memset(&(server_addr.sin_zero), 0, 8);    // Zero out the rest of the struct (padding)

    // 4. Bind the socket to the given IP/port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    // 5. Start listening for incoming connections
    // "5" here is the backlog queue size for pending connections
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(1);
    }
    printf("HTTP Server running on port 8080...\n");

    // Server main loop — handles one client at a time
    while (1) {
        // 6. Accept a client connection (blocking call)
        addr_size = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
        if (client_fd < 0) {
            perror("Accept failed");
            continue; // Skip to the next loop iteration if accept fails
        }

        // 7. Display the client's IP and port
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Client connected: %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        // 8. Clear buffer before reading (avoids leftover data)
        memset(buffer, 0, sizeof(buffer));

        // 9. Read HTTP request from client
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1); // Leave 1 byte for null terminator
        if (bytes_read > 0) buffer[bytes_read] = '\0'; // Null-terminate the request string
        printf("Client Request:\n%s\n", buffer);

        // 10. Prepare a simple HTTP response with HTML content
        char *html = "<h1>Hello from C HTTP Server</h1>";
        char response[1024];
        sprintf(response,
                "HTTP/1.1 200 OK\r\n"             // Status line
                "Content-Type: text/html\r\n"     // Content type
                "Content-Length: %ld\r\n"         // Content length
                "\r\n"                            // Blank line to separate headers from body
                "%s",                             // HTML body
                strlen(html), html);

        // 11. Send HTTP response to the client
        write(client_fd, response, strlen(response));

        // 12. Close connection with the client
        close(client_fd);
    }

    // Close the main server socket (this point is never reached in normal operation)
    close(server_fd);
    return 0;
}


// Key Features of This Code:
// Graceful shutdown with Ctrl+C using signal(SIGINT, handle_sigint).
// Port reuse enabled via setsockopt() to avoid waiting when restarting the server.
// Handles basic HTTP GET requests and sends a fixed HTML page.
// Prints the client's IP and port when they connect.
// Simple loop to handle multiple clients sequentially.