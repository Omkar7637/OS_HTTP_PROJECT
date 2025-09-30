#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>         // for close()
#include <arpa/inet.h>      // for sockaddr_in, htons, etc.
#include <time.h>           // for time() and ctime()

#define PORT 8080           // Port where our HTTP server will run
#define BUF_SIZE 4096       // Buffer size for reading requests

// ====================== FUNCTION: recv_all_headers ======================
// This function reads the full HTTP request headers from the client socket.
// - It keeps calling recv() until it finds the end of headers ("\r\n\r\n").
// - It also prints how many times recv() was called.
// - Returns the total number of bytes read.
// ========================================================================
int recv_all_headers(int client_fd, char *buf, size_t size) {
    int total = 0, n, recv_count = 0;

    // Keep receiving data until headers are fully received
    while ((n = recv(client_fd, buf + total, size - total - 1, 0)) > 0) {
        recv_count++;
        total += n;
        buf[total] = '\0';  // Null-terminate so strstr works safely

        // HTTP request headers end with a blank line (\r\n\r\n)
        if (strstr(buf, "\r\n\r\n")) break;
    }

    // Print statistics and raw request for debugging
    printf("recv() was called %d times to read headers.\n", recv_count);
    printf("===== RAW HTTP REQUEST =====\n%s\n============================\n", buf);

    return total;
}

// ====================== FUNCTION: handle_client =========================
// This function handles one connected client.
// Steps:
//   1. Receive full request headers
//   2. Parse method, path, and version from request line
//   3. Reject unsupported methods (only GET allowed here)
//   4. Parse query string if present (after '?')
//   5. Parse Host header
//   6. Serve different responses based on path
//   7. Build HTTP response and send it back
// =========================================================================
void handle_client(int client_fd) {
    char buf[BUF_SIZE];

    // Step 1: Receive headers
    recv_all_headers(client_fd, buf, sizeof(buf));

    // Step 2: Parse the request line (e.g., "GET /hello HTTP/1.1")
    char method[16], path[256], version[16];
    if (sscanf(buf, "%s %s %s", method, path, version) != 3) {
        close(client_fd);
        return; // Malformed request
    }

    // Step 3: Reject unsupported methods (only GET supported)
    if (strcmp(method, "GET") != 0) {
        char *resp = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        send(client_fd, resp, strlen(resp), 0);
        close(client_fd);
        return;
    }

    // Step 4: Handle Query Parameters (everything after '?')
    char *query = NULL;
    char *qmark = strchr(path, '?');
    if (qmark) {
        *qmark = '\0';   // Cut the path at '?'
        query = qmark + 1;
    }

    // Print parsed request details
    printf("Method=%s\n", method);
    printf("Path=%s\n", path);
    printf("Version=%s\n", version);
    if (query) {
        printf("Query=%s\n", query);
    }

    // Step 5: Parse the "Host" header
    char *host_ptr = strstr(buf, "Host:");
    if (host_ptr) {
        char host_val[256];
        sscanf(host_ptr, "Host: %255s", host_val);
        printf("Host=%s\n", host_val);
    }

    // Step 6: Generate different responses based on requested path
    char body[1024];
    if (strcmp(path, "/hello") == 0) {
        snprintf(body, sizeof(body), "Hello Student!");
    } else if (strcmp(path, "/time") == 0) {
        time_t now = time(NULL);
        snprintf(body, sizeof(body), "Current Time: %s", ctime(&now));
    } else {
        snprintf(body, sizeof(body), "Unknown Path!");
        if (query) {
            // Optional: include query in response to show handling
            strncat(body, "\nQuery=", sizeof(body) - strlen(body) - 1);
            strncat(body, query, sizeof(body) - strlen(body) - 1);
        }
    }

    // Step 7: Build and send HTTP response
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

    // Close client connection after response
    close(client_fd);
}

// =========================== MAIN FUNCTION ==============================
// Main server loop:
//   1. Create socket
//   2. Allow address reuse (SO_REUSEADDR)
//   3. Bind socket to IP/port
//   4. Listen for incoming connections
//   5. Accept clients in a loop
//   6. Pass each client to handle_client()
// =========================================================================
int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    // Step 1: Create a TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    // Step 2: Allow reusing the port quickly after restart
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Step 3: Setup server address (IPv4, any interface, PORT)
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    // Bind socket to the chosen port
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    // Step 4: Put socket into listening mode
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Server running on http://localhost:%d\n", PORT);

    // Step 5: Accept and handle clients in a loop
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&addr, &addrlen);
        if (client_fd < 0) { perror("accept"); continue; }

        // Step 6: Handle each client request
        handle_client(client_fd);
    }

    // Cleanup (not reached in infinite loop normally)
    close(server_fd);
    return 0;
}
