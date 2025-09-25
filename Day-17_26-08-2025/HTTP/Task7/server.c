#include <stdio.h>      // Standard I/O functions
#include <stdlib.h>     // Standard library (exit, malloc, etc.)
#include <string.h>     // String functions (strcpy, strstr, strlen, etc.)
#include <unistd.h>     // For close(), read(), write(), etc.
#include <arpa/inet.h>  // For socket functions and structures
#include <time.h>       // For getting current time

#define PORT 8080       // Port number where server will listen
#define BUF_SIZE 4096   // Maximum buffer size for request/response

/*
 * recv_all_headers:
 * Reads HTTP headers from the client socket until it reaches
 * the end of headers ("\r\n\r\n").
 * Keeps track of how many times recv() was called.
 * Prints the raw HTTP request headers.
 */
int recv_all_headers(int client_fd, char *buf, size_t size) {
    int total = 0, n, recv_count = 0;

    // Keep reading from socket until end of headers is found
    while ((n = recv(client_fd, buf + total, size - total - 1, 0)) > 0) {
        recv_count++;
        total += n;
        buf[total] = '\0';  // Null terminate buffer
        if (strstr(buf, "\r\n\r\n")) break; // End of headers found
    }

    // Debug info
    printf("recv() was called %d times to read headers.\n", recv_count);
    printf("===== RAW HTTP REQUEST =====\n%s\n============================\n", buf);

    return total;
}

/*
 * handle_client:
 * Handles a single client connection.
 * - Reads headers
 * - Parses request line (method, path, version)
 * - Handles GET and POST requests
 * - Sends proper HTTP response
 */
void handle_client(int client_fd) {
    char buf[BUF_SIZE];

    // Step 1: Receive all HTTP headers
    recv_all_headers(client_fd, buf, sizeof(buf));

    // --- Parse request line (first line: METHOD PATH VERSION) ---
    char method[16], path[256], version[16];
    if (sscanf(buf, "%s %s %s", method, path, version) != 3) {
        close(client_fd);  // Invalid request
        return;
    }

    // --- Detect Content-Length for POST request ---
    int content_length = 0;
    char *cl_ptr = strcasestr(buf, "Content-Length:");
    if (cl_ptr) {
        sscanf(cl_ptr, "Content-Length: %d", &content_length);
    }

    // --- Handle query parameters if present ---
    char *query = NULL;
    char *qmark = strchr(path, '?');  // Find '?'
    if (qmark) {
        *qmark = '\0';     // Split path and query
        query = qmark + 1;
    }

    // Debug info
    printf("Method=%s\n", method);
    printf("Path=%s\n", path);
    printf("Version=%s\n", version);
    if (query) {
        printf("Query=%s\n", query);
    }

    // --- Parse Host header ---
    char *host_ptr = strstr(buf, "Host:");
    if (host_ptr) {
        char host_val[256];
        sscanf(host_ptr, "Host: %255s", host_val);
        printf("Host=%s\n", host_val);
    }

    // --- Handle GET Request ---
    if (strcmp(method, "GET") == 0) {
        char body[1024];

        // Example endpoints
        if (strcmp(path, "/hello") == 0) {
            snprintf(body, sizeof(body), "Hello Student!");
        } else if (strcmp(path, "/time") == 0) {
            time_t now = time(NULL);
            snprintf(body, sizeof(body), "Current Time: %s", ctime(&now));
        } else {
            // Unknown path
            snprintf(body, sizeof(body), "Unknown Path!");
            if (query) {
                // Append query to response
                strncat(body, "\nQuery=", sizeof(body) - strlen(body) - 1);
                strncat(body, query, sizeof(body) - strlen(body) - 1);
            }
        }

        // Build HTTP response
        char response[2048];
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Length: %lu\r\n"
                 "Content-Type: text/plain\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "%s",
                 strlen(body), body);

        send(client_fd, response, strlen(response), 0);
    }

    // --- Handle POST Request ---
    else if (strcmp(method, "POST") == 0) {
        char *body_start = strstr(buf, "\r\n\r\n");  // Find body
        char body_data[BUF_SIZE] = {0};
        int already_have = 0;

        if (body_start) {
            body_start += 4;  // Skip CRLFCRLF
            already_have = strlen(body_start);
            strncpy(body_data, body_start, already_have);
        }

        // If full body not yet read, receive remaining
        int remaining = content_length - already_have;
        int n;
        while (remaining > 0 && (n = recv(client_fd, body_data + already_have, remaining, 0)) > 0) {
            already_have += n;
            remaining -= n;
        }
        body_data[already_have] = '\0';

        // Print POST body for debugging
        printf("===== POST BODY =====\n%s\n=====================\n", body_data);

        // Build header response (without body yet)
        char header[256];
        int header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %lu\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n",
            strlen(body_data) + 20); // +20 for extra text

        send(client_fd, header, header_len, 0);

        // Send body separately
        char body_msg[4096];
        int body_len = snprintf(body_msg, sizeof(body_msg),
                                "Received POST data:\n%s", body_data);
        send(client_fd, body_msg, body_len, 0);
    }

    // --- Handle Unsupported Methods ---
    else {
        char *resp = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        send(client_fd, resp, strlen(resp), 0);
    }

    // Close connection
    close(client_fd);
}

/*
 * main:
 * - Creates a TCP socket
 * - Binds to localhost:8080
 * - Listens for incoming clients
 * - Accepts and handles clients one by one
 */
int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    // Allow reusing port immediately after restart
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Setup server address
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;  // Bind to all interfaces
    addr.sin_port = htons(PORT);        // Convert port to network byte order

    // Bind socket to address and port
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    // Listen for incoming connections (queue size = 10)
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Server running on http://localhost:%d\n", PORT);

    // Accept and handle clients in loop
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&addr, &addrlen);
        if (client_fd < 0) { perror("accept"); continue; }
        handle_client(client_fd);
    }

    // Close server (unreachable in this infinite loop)
    close(server_fd);
    return 0;
}
