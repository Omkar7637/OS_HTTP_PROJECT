#include <stdio.h>              // For printf(), perror() — basic I/O operations
#include <stdlib.h>             // For exit(), EXIT_FAILURE — process control and exit codes
#include <string.h>             // For memset(), strlen(), sprintf() — memory and string utilities
#include <unistd.h>             // For close(), read(), write() — POSIX system calls
#include <arpa/inet.h>          // For inet_ntop(), htons(), INET_ADDRSTRLEN — networking utilities
#include <signal.h>             // For signal() — handling Ctrl+C (SIGINT) to gracefully stop server
#include <sys/socket.h>         // For socket(), bind(), listen(), accept() — core socket API
#include <netinet/in.h>         // For sockaddr_in — structure for IPv4 addresses

#define PORT 8080                // Define server port (easier to change later)

// Global server socket descriptor — needed so the signal handler can close it
int server_fd; 

// -------------------- Signal Handler --------------------
// This runs when user presses Ctrl+C in the terminal
// Purpose: Close the socket before exiting so the port is released
void handle_sigint(int sig) {
    printf("\nShutting down server...\n");
    close(server_fd);  // Free the port
    exit(0);
}

// -------------------- File Sending Function --------------------
// Purpose: Send an HTML file to the client as part of the HTTP response
void send_file(int client_fd, const char *filename) {
    FILE *fp = fopen(filename, "r");  // Open file in read mode
    if (!fp) {
        // If file not found, send a 404 error page
        char *not_found = "HTTP/1.1 404 Not Found\r\n"
                          "Content-Type: text/html\r\n\r\n"
                          "<h1>404 - File Not Found</h1>";
        write(client_fd, not_found, strlen(not_found));
        return;
    }

    // Send HTTP response header for a successful request
    char header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    write(client_fd, header, strlen(header));

    // Read and send file content in chunks
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp)) {
        write(client_fd, buffer, strlen(buffer));
    }

    fclose(fp); // Close file after sending
}

// -------------------- Main Function --------------------
int main() {
    int client_fd;                               // Socket descriptor for client
    struct sockaddr_in server_addr, client_addr; // Store server & client addresses
    socklen_t addr_size;                         // Holds client address size
    char buffer[1024];                           // Temporary buffer for incoming data

    // 1. Register signal handler to capture Ctrl+C
    signal(SIGINT, handle_sigint);

    // 2. Create a TCP socket
    // AF_INET      = IPv4
    // SOCK_STREAM  = TCP (connection-oriented)
    // Protocol 0   = Use default for SOCK_STREAM (TCP)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 3. Allow port reuse immediately after closing server
    // Set SO_REUSEADDR to reuse the port immediately after closing
    // This avoids the "Address already in use" error during quick restarts

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // 4. Configure server address
    server_addr.sin_family = AF_INET;         // IPv4
    server_addr.sin_port = htons(PORT);       // Convert to network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any network interface
    memset(&(server_addr.sin_zero), 0, 8);    // Zero padding

    // 5. Bind socket to the port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 6. Start listening for incoming connections
    if (listen(server_fd, 5) < 0) { // "5" is the max backlog of pending connections
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    printf("HTTP Server running on port %d...\n", PORT);

    // -------------------- Main Server Loop --------------------
    while (1) {
        // 7. Accept a client connection
        addr_size = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
        if (client_fd < 0) {
            perror("Accept failed");
            continue; // Skip to the next loop iteration if accept fails
        }

        // 8. Display client's IP and port
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Client connected: %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        // 9. Clear buffer and read HTTP request
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) buffer[bytes_read] = '\0';
        printf("Client Request:\n%s\n", buffer);

        // 10. Serve HTML file (index.html)
        send_file(client_fd, "index.html");

        // 11. Close connection to client
        close(client_fd);
    }

    // (Never reached unless loop breaks)
    // Close the main server socket (this point is never reached in normal operation)
    close(server_fd);
    return 0;
}

/*
================= Key Features =================
1. Graceful Shutdown:
   - Captures SIGINT (Ctrl+C) to close socket before exiting.
2. Port Reuse:
   - Uses setsockopt() with SO_REUSEADDR to prevent "Address already in use".
3. File Serving:
   - Can send "index.html" or return a 404 if missing.
4. Client Info Display:
   - Prints client's IP and port when connected.
5. Basic HTTP Handling:
   - Reads HTTP GET request and sends simple HTML response.
6. Single-threaded Sequential Handling:
   - Handles one client at a time (can be upgraded to multi-threaded later).
*/
