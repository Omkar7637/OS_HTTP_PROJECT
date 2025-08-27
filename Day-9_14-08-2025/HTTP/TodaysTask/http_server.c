#include <stdio.h>              // For printf, perror, snprintf
#include <stdlib.h>             // For exit, EXIT_FAILURE
#include <string.h>             // For memset, strcmp, sscanf
#include <unistd.h>             // For close, read, write
#include <arpa/inet.h>          // For inet_ntop, htons, INET_ADDRSTRLEN
#include <signal.h>             // For signal handling (SIGINT)
#include <sys/socket.h>         // For socket, bind, listen, accept
#include <netinet/in.h>         // For sockaddr_in
#include <time.h>               // For time, localtime, strftime

#define PORT 8080                // Server will listen on port 8080

int server_fd;                   // Global socket descriptor for the server

// =======================
// Signal handler for Ctrl+C
// =======================
void handle_sigint(int sig) {
    printf("\nShutting down server...\n");
    close(server_fd);           // Close listening socket to free the port
    exit(0);                    // Exit program
}

int main() {
    int client_fd;                                   // Socket descriptor for client
    struct sockaddr_in server_addr, client_addr;    // Structures for server & client address
    socklen_t addr_size;                             // Holds size of client address
    char buffer[4096];                               // Large buffer to store HTTP request

    // Handle Ctrl+C so server shuts down cleanly
    signal(SIGINT, handle_sigint);

    // 1. Create a TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. Set socket options so we can reuse the address immediately after restart
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. Configure the server address
    server_addr.sin_family = AF_INET;               // IPv4
    server_addr.sin_port = htons(PORT);             // Port in network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY;       // Listen on all available network interfaces
    memset(&(server_addr.sin_zero), 0, 8);          // Zero padding for struct alignment

    // 4. Bind the socket to the IP & port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 5. Start listening for incoming connections
    if (listen(server_fd, 5) < 0) { // "5" is the backlog (max pending connections)
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("HTTP Server running on port %d...\n", PORT);

    // =======================
    // Main server loop
    // =======================
    while (1) {
        addr_size = sizeof(client_addr);

        // 6. Accept a new client connection
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
        if (client_fd < 0) {
            perror("Accept failed");
            continue; // Skip this iteration if accept fails
        }

        // 7. Get client IP address in human-readable form
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Client connected: %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        // 8. Read full HTTP request from client
        memset(buffer, 0, sizeof(buffer)); // Clear buffer
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) buffer[bytes_read] = '\0'; // Null-terminate string

        // 9. Log the full raw HTTP request
        printf("===== RAW HTTP REQUEST =====\n%s\n============================\n", buffer);

        // 10. Parse HTTP method and path from request line (e.g., "GET /path HTTP/1.1")
        char method[8], path[256];
        sscanf(buffer, "%s %s", method, path);

        // 11. Prepare dynamic data: Current date/time
        char date_str[64];
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", t);

        // 12. Common HTTP header
        const char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
        char response[4096];

        // 13. Serve different HTML pages depending on the request path
        if (strcmp(path, "/hello") == 0) {
            snprintf(response, sizeof(response),
                     "%s"
                     "<html><body><h1>Hello Page</h1>"
                     "<p>Client IP: %s</p>"
                     "<p>Current Time: %s</p>"
                     "</body></html>",
                     header, client_ip, date_str);
        }
        else if (strcmp(path, "/bye") == 0) {
            snprintf(response, sizeof(response),
                     "%s"
                     "<html><body><h1>Goodbye Page</h1>"
                     "<p>Client IP: %s</p>"
                     "<p>Current Time: %s</p>"
                     "</body></html>",
                     header, client_ip, date_str);
        }
        else {
            snprintf(response, sizeof(response),
                     "%s"
                     "<html><body><h1>Default Page</h1>"
                     "<p>Client IP: %s</p>"
                     "<p>Current Time: %s</p>"
                     "</body></html>",
                     header, client_ip, date_str);
        }

        // 14. Send the HTTP response to the client
        write(client_fd, response, strlen(response));

        // 15. Close connection with this client
        close(client_fd);
    }

    // Close server socket (usually never reached unless loop breaks)
    close(server_fd);
    return 0;
}
