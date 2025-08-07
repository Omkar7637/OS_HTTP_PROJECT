#include <stdio.h>              // For input/output functions like printf, perror
#include <stdlib.h>             // For exit() and general utilities
#include <string.h>             // For memset(), strlen()
#include <unistd.h>             // For close(), write(), read()
#include <sys/socket.h>         // For socket(), bind(), listen(), accept(), recv()
#include <netinet/in.h>         // For sockaddr_in and IP-related constants
#include <arpa/inet.h>          // For inet_ntop() to convert IP to readable form

int main() {
    int server_fd, client_fd;                         // File descriptors for server and client socket
    struct sockaddr_in server_addr, client_addr;      // Structures to store address info
    socklen_t addr_size;                              // To store size of address
    char buffer[1024];                                // Buffer to hold client message
    char client_ip[INET_ADDRSTRLEN];                  // To store client's IP in text format

    // 1. Create TCP socket
    // AF_INET: Address family for IPv4
    // SOCK_STREAM: TCP socket (stream-oriented)
    // 0: Protocol set to default (TCP in this case)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // 2. Set up server address structure
    // Fill in server address structure with IP/port info
    server_addr.sin_family = AF_INET;                // IPv4
    server_addr.sin_port = htons(8080);              // Port 8080 (converted to network byte order)
    server_addr.sin_addr.s_addr = INADDR_ANY;        // Accept connections on any local IP address
    memset(&(server_addr.sin_zero), 0, 8);           // Zero the rest of the structure (not always necessary)

    // This allows your server to reuse the port immediately after it's closed, avoiding the "Address already in use" error during development.
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }


    // 3. Bind the socket to the specified IP and port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);                            // Clean up if bind fails
        exit(1);
    }

    // 4. Listen for incoming connections
    // Second argument (5) is backlog: max pending connections queue length
    listen(server_fd, 5);
    printf("Server listening on port 8080...\n");

    // 5. Accept incoming connection from client
    // This is a blocking call — it waits until a client connects
    addr_size = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
    if (client_fd < 0) {
        perror("Accept failed");
        close(server_fd);
        exit(1);
    }

    // Convert client IP address to human-readable string
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    printf("Client connected from %s:%d\n", client_ip, ntohs(client_addr.sin_port));

    // Send welcome message to client
    char *welcome_msg = "Welcome to the server!\n";
    if (write(client_fd, welcome_msg, strlen(welcome_msg)) < 0) {
        perror("Write to client failed");
    }

    // 6. Start receiving messages from client
    // Loop continues until client disconnects or error occurs
    while (1) {
        memset(buffer, 0, sizeof(buffer)); // Clear the buffer before receiving new data
        int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0); // Receive data
        if (bytes_received <= 0) {
            // If client disconnects or error occurs
            printf("Client disconnected or error occurred.\n");
            break;
        }
        if (strncmp(buffer, "exit", 4) == 0 || strncmp(buffer, "bye", 3) == 0) {
        printf("Client requested to close the connection.\n");
        break;
}
        printf("Client says: %s\n", buffer); // Print received message
    }

    // 7. Clean up and close sockets
    close(client_fd);     // Close client connection
    close(server_fd);     // Close server socket
    return 0;
}
