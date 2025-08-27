#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define PORT 8080

// ---------------- recv_all_headers() ----------------
int recv_all_headers(int client_fd, char *buffer, size_t size) {
    int total_bytes = 0;
    int recv_calls = 0;
    int bytes;

    while (1) {
        bytes = recv(client_fd, buffer + total_bytes, size - total_bytes - 1, 0);
        recv_calls++;

        if (bytes <= 0) break;
        total_bytes += bytes;
        buffer[total_bytes] = '\0';

        if (strstr(buffer, "\r\n\r\n")) {
            break; // end of headers
        }
    }

    printf("===== RAW HTTP REQUEST =====\n%s\n============================\n", buffer);
    printf("recv() was called %d times to read headers.\n", recv_calls);

    return total_bytes;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buffer[4096];

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }

    listen(server_fd, 5);
    printf("Server listening on port %d...\n", PORT);

    while (1) {
        addr_size = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);

        memset(buffer, 0, sizeof(buffer));
        recv_all_headers(client_fd, buffer, sizeof(buffer));

        // ---------------- Task 2: Extract Method, Path, Version ----------------
        char method[16], path[256], version[16];
        if (sscanf(buffer, "%15s %255s %15s", method, path, version) == 3) {
            printf("Method: %s\n", method);
            printf("Path: %s\n", path);
            printf("Version: %s\n", version);

            // Only allow GET
            if (strcmp(method, "GET") != 0) {
                char *resp_405 =
                    "HTTP/1.1 405 Method Not Allowed\r\n"
                    "Content-Type: text/html\r\n\r\n"
                    "<h1>405 - Method Not Allowed</h1>";
                send(client_fd, resp_405, strlen(resp_405), 0);
                close(client_fd);
                continue;
            }
        } else {
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
            host_ptr += 5; // skip "Host:"
            while (*host_ptr == ' ') host_ptr++; // skip spaces

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

        // ---------------- Task 4: Serve Different Paths ----------------
        if (strcmp(path, "/hello") == 0) {
            char *resp_hello =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n\r\n"
                "<h1>Hello Student!</h1>";
            send(client_fd, resp_hello, strlen(resp_hello), 0);
        }
        else if (strcmp(path, "/time") == 0) {
            time_t now = time(NULL);
            char *time_str = ctime(&now); // includes newline
            char body[256];
            snprintf(body, sizeof(body), "<h1>Current Time: %s</h1>", time_str);

            char resp_time[512];
            snprintf(resp_time, sizeof(resp_time),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n\r\n"
                "%s", body);

            send(client_fd, resp_time, strlen(resp_time), 0);
        }
        else {
            char *resp_unknown =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html\r\n\r\n"
                "<h1>Unknown Path!</h1>";
            send(client_fd, resp_unknown, strlen(resp_unknown), 0);
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
