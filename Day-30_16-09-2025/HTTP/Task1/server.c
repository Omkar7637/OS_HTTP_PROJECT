#include <stdio.h>      // Standard I/O functions (printf, perror etc.)
#include <stdlib.h>     // For exit(), malloc(), free() etc.
#include <string.h>     // For string operations like memset(), strstr(), strlen()
#include <unistd.h>     // For close(), read(), write() functions
#include <arpa/inet.h>  // For inet_ntop(), htons(), htonl() etc.
#include <sys/socket.h> // For socket(), bind(), listen(), accept(), recv(), send()
#include <netinet/in.h> // For sockaddr_in structure (IPv4 addressing)

// Define server port (browser will connect using http://localhost:8080)
#define PORT 8080  

// ============================================================
// Function: recv_all_headers()
// Purpose : Reads the entire HTTP request headers from client
//           using multiple recv() calls if required.
//
// Parameters:
//   client_fd -> socket file descriptor of client
//   buffer    -> char array to store request data
//   size      -> size of buffer
//
// Working:
//   - HTTP requests arrive as raw text, possibly in fragments
//   - recv() reads available bytes (may not be complete request)
//   - Loop keeps calling recv() until "\r\n\r\n" (end of headers)
// ============================================================
int recv_all_headers(int client_fd, char *buffer, size_t size) {
    int total_bytes = 0;   // Total bytes received so far
    int recv_calls = 0;    // Count how many times recv() was called
    int bytes;             // Bytes received in each recv() call

    while (1) {
        // recv() reads data from client socket into buffer
        // Arguments:
        //   client_fd -> client connection socket
        //   buffer + total_bytes -> position to store new data
        //   size - total_bytes - 1 -> remaining space in buffer
        //   0 -> no special flags
        bytes = recv(client_fd, buffer + total_bytes, size - total_bytes - 1, 0);
        recv_calls++;  // Track how many times recv() was called

        if (bytes <= 0) break;  // Client closed connection or error
        total_bytes += bytes;   // Update total bytes received
        buffer[total_bytes] = '\0'; // Null terminate to make it valid C-string

        // Check if end of HTTP headers reached (marked by "\r\n\r\n")
        if (strstr(buffer, "\r\n\r\n")) {
            break;  // Stop reading once headers complete
        }
    }

    // Print raw request for debugging
    printf("===== RAW HTTP REQUEST =====\n%s\n============================\n", buffer);
    printf("recv() was called %d times to read headers.\n", recv_calls);

    return total_bytes; // Return total data length received
}

int main() {
    int server_fd, client_fd;             // Socket file descriptors
    struct sockaddr_in server_addr;       // Server address structure
    struct sockaddr_in client_addr;       // Client address structure
    socklen_t addr_size;                  // Size of client address structure
    char buffer[4096];                    // Buffer to hold request

    // ---------------- STEP 1: Create Socket ----------------
    // socket(domain, type, protocol)
    // domain   = AF_INET  (IPv4)
    // type     = SOCK_STREAM (TCP)
    // protocol = 0 (default TCP)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // ---------------- STEP 2: Set Socket Options ----------------
    // SO_REUSEADDR allows reusing the same port immediately after restart
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // ---------------- STEP 3: Bind Socket ----------------
    // Fill server address details
    server_addr.sin_family = AF_INET;          // IPv4
    server_addr.sin_port = htons(PORT);        // Port (converted to network byte order)
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Accept connections from any IP

    // Bind server_fd to given IP/Port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }

    // ---------------- STEP 4: Listen ----------------
    // Put socket in passive mode (ready to accept connections)
    listen(server_fd, 5); // Queue size = 5
    printf("Server listening on port %d...\n", PORT);

    // ---------------- STEP 5: Accept & Handle Clients ----------------
    while (1) {
        addr_size = sizeof(client_addr);

        // accept() blocks until a client connects
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        // Clear buffer before using
        memset(buffer, 0, sizeof(buffer));

        // Read full HTTP request headers
        recv_all_headers(client_fd, buffer, sizeof(buffer));

        // ---------------- STEP 6: Send Response ----------------
        // Send a minimal valid HTTP response
        // Format: HTTP/1.1 <status>\r\n<header>\r\n\r\n<body>
        char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "Task 1 Done"; // Response body

        send(client_fd, response, strlen(response), 0);

        // ---------------- STEP 7: Close Connection ----------------
        close(client_fd);
    }

    // ---------------- STEP 8: Close Server Socket ----------------
    close(server_fd);
    return 0;
}
