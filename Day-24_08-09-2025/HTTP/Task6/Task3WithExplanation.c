/*
==============================================
    Simple HTTP Server with Detailed Comments
==============================================

This program demonstrates how to build a very 
basic HTTP server in C. It listens on port 8080,
accepts client connections, parses the HTTP 
request headers, extracts the request line 
(Method, Path, Version), checks the "Host" header,
and then responds with a simple HTML message.

We will explain each header file, each function,
and each parameter in detail.
*/

#include <stdio.h>      // printf(), perror(), sscanf() — Standard I/O library
#include <stdlib.h>     // exit(), malloc(), free() — General utilities
#include <string.h>     // memset(), strcmp(), strstr(), strlen() — String handling
#include <unistd.h>     // close(), read(), write() — UNIX standard functions
#include <arpa/inet.h>  // htons(), inet_ntoa(), inet_addr() — Functions for internet operations
#include <sys/socket.h> // socket(), bind(), listen(), accept(), send(), recv() — Core socket API
#include <netinet/in.h> // sockaddr_in, INADDR_ANY — Structures and macros for internet domain

#define PORT 8080   // Server will listen on TCP port 8080

/* =======================================================
   Function: recv_all_headers
   Purpose:  Read all HTTP request headers from client.
   Params:
      - client_fd : File descriptor of connected client socket
      - buffer    : Pointer to memory where received data is stored
      - size      : Size of buffer (to avoid overflow)
   Returns:
      - total number of bytes read from client
   ======================================================= */
int recv_all_headers(int client_fd, char *buffer, size_t size) {
    int total_bytes = 0;   // Total data read from client
    int recv_calls = 0;    // Count how many times recv() was called
    int bytes;             // Number of bytes read in one recv()

    while (1) {
        // recv() — read from socket
        // Params: 
        //   client_fd   = socket descriptor
        //   buffer + total_bytes = where to store new data
        //   size - total_bytes - 1 = remaining buffer capacity
        //   0 = flags (blocking mode)
        bytes = recv(client_fd, buffer + total_bytes, size - total_bytes - 1, 0);
        recv_calls++;

        if (bytes <= 0) break; // if no more data, break loop

        total_bytes += bytes;
        buffer[total_bytes] = '\0'; // Null-terminate string for safety

        // Check if "\r\n\r\n" (end of HTTP headers) is found
        if (strstr(buffer, "\r\n\r\n")) {
            break; 
        }
    }

    printf("===== RAW HTTP REQUEST =====\n%s\n============================\n", buffer);
    printf("recv() was called %d times to read headers.\n", recv_calls);

    return total_bytes;
}

int main() {
    int server_fd, client_fd;                 // File descriptors for server & client sockets
    struct sockaddr_in server_addr, client_addr; // Structures to store server/client addresses
    socklen_t addr_size;                      // Size of client address structure
    char buffer[4096];                        // Buffer to store client request

    // ---------------- Step 1: Create Socket ----------------
    // socket(domain, type, protocol)
    //   AF_INET  = IPv4
    //   SOCK_STREAM = TCP
    //   0 = default protocol for TCP
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket failed");
        exit(1);
    }

    // setsockopt() allows reuse of the port immediately after closing server
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // ---------------- Step 2: Setup Server Address ----------------
    server_addr.sin_family = AF_INET;           // IPv4
    server_addr.sin_port = htons(PORT);         // Port number in network byte order (htons converts host → network)
    server_addr.sin_addr.s_addr = INADDR_ANY;   // Accept connections on any local IP address

    // ---------------- Step 3: Bind Socket ----------------
    // bind() assigns IP + Port to socket
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }

    // ---------------- Step 4: Start Listening ----------------
    // listen(socket, backlog)
    // backlog = max number of queued connections
    listen(server_fd, 5);
    printf("Server listening on port %d...\n", PORT);

    // ---------------- Step 5: Accept Clients ----------------
    while (1) {
        addr_size = sizeof(client_addr);

        // accept() returns new socket for each client
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);

        // Clear buffer before receiving new request
        memset(buffer, 0, sizeof(buffer));

        // Step 6: Read full HTTP headers from client
        recv_all_headers(client_fd, buffer, sizeof(buffer));

        // ---------------- Task 2: Extract Method, Path, Version ----------------
        char method[16], path[256], version[16];
        // sscanf extracts first line: "METHOD PATH VERSION"
        if (sscanf(buffer, "%15s %255s %15s", method, path, version) == 3) {
            printf("Method: %s\n", method);
            printf("Path: %s\n", path);
            printf("Version: %s\n", version);

            // Restrict to only GET method
            if (strcmp(method, "GET") != 0) {
                char *resp_405 =
                    "HTTP/1.1 405 Method Not Allowed\r\n"
                    "Content-Type: text/html\r\n\r\n"
                    "<h1>405 - Method Not Allowed</h1>";
                send(client_fd, resp_405, strlen(resp_405), 0);
                close(client_fd);
                continue; // go back and wait for next client
            }
        } else {
            // Invalid request line → respond with 400 Bad Request
            char *resp_400 =
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Type: text/html\r\n\r\n"
                "<h1>400 - Bad Request</h1>";
            send(client_fd, resp_400, strlen(resp_400), 0);
            close(client_fd);
            continue;
        }

        // ---------------- Task 3: Parse Host Header ----------------
        char *host_ptr = strstr(buffer, "Host:");
        if (host_ptr) {
            host_ptr += 5; // Skip "Host:"
            while (*host_ptr == ' ') host_ptr++; // Skip spaces

            char host_value[256];
            int i = 0;
            while (*host_ptr && *host_ptr != '\r' && *host_ptr != '\n' && i < 255) {
                host_value[i++] = *host_ptr++;
            }
            host_value[i] = '\0';

            printf("Host = %s\n", host_value);
        } else {
            printf("Host header not found!\n");
        }

        // ---------------- Step 7: Send Response ----------------
        char *resp_ok =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<h1>Task 3 Done</h1>";
        send(client_fd, resp_ok, strlen(resp_ok), 0);

        // Close connection with this client
        close(client_fd);
    }

    // Close server socket (never reached in this infinite loop)
    close(server_fd);
    return 0;
}
