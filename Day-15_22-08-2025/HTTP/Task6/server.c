#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define BUF_SIZE 4096

// recv_all_headers: reads HTTP headers and prints them
int recv_all_headers(int client_fd, char *buf, size_t size) {
    int total = 0, n, recv_count = 0;
    while ((n = recv(client_fd, buf + total, size - total - 1, 0)) > 0) {
        recv_count++;
        total += n;
        buf[total] = '\0';
        if (strstr(buf, "\r\n\r\n")) break; // end of headers
    }
    printf("recv() was called %d times to read headers.\n", recv_count);

    printf("===== RAW HTTP REQUEST =====\n%s\n============================\n", buf);
    return total;
}

void handle_client(int client_fd) {
    char buf[BUF_SIZE];
    recv_all_headers(client_fd, buf, sizeof(buf));

    // --- Parse request line ---
    char method[16], path[256], version[16];
    if (sscanf(buf, "%s %s %s", method, path, version) != 3) {
        close(client_fd);
        return;
    }

    // --- Detect Content-Length for POST ---
    int content_length = 0;
    char *cl_ptr = strcasestr(buf, "Content-Length:");
    if (cl_ptr) {
        sscanf(cl_ptr, "Content-Length: %d", &content_length);
    }

    // --- Handle Query Parameters ---
    char *query = NULL;
    char *qmark = strchr(path, '?');
    if (qmark) {
        *qmark = '\0';
        query = qmark + 1;
    }

    printf("Method=%s\n", method);
    printf("Path=%s\n", path);
    printf("Version=%s\n", version);
    if (query) {
        printf("Query=%s\n", query);
    }

    // --- Parse Host Header ---
    char *host_ptr = strstr(buf, "Host:");
    if (host_ptr) {
        char host_val[256];
        sscanf(host_ptr, "Host: %255s", host_val);
        printf("Host=%s\n", host_val);
    }

    // --- Handle GET Request ---
    if (strcmp(method, "GET") == 0) {
        char body[1024];
        if (strcmp(path, "/hello") == 0) {
            snprintf(body, sizeof(body), "Hello Student!");
        } else if (strcmp(path, "/time") == 0) {
            time_t now = time(NULL);
            snprintf(body, sizeof(body), "Current Time: %s", ctime(&now));
        } else {
            snprintf(body, sizeof(body), "Unknown Path!");
            if (query) {
                strncat(body, "\nQuery=", sizeof(body) - strlen(body) - 1);
                strncat(body, query, sizeof(body) - strlen(body) - 1);
            }
        }

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
        char *body_start = strstr(buf, "\r\n\r\n");
        char body_data[BUF_SIZE] = {0};
        int already_have = 0;

        if (body_start) {
            body_start += 4; // skip CRLFCRLF
            already_have = strlen(body_start);
            strncpy(body_data, body_start, already_have);
        }

        // If headers didn’t read the whole body, recv() the rest
        int remaining = content_length - already_have;
        int n;
        while (remaining > 0 && (n = recv(client_fd, body_data + already_have, remaining, 0)) > 0) {
            already_have += n;
            remaining -= n;
        }
        body_data[already_have] = '\0';

        printf("===== POST BODY =====\n%s\n=====================\n", body_data);

        // --- Build header only ---
        char header[256];
        int header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %lu\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n",
            strlen(body_data) + 20);

        // Send header
        send(client_fd, header, header_len, 0);

        // --- Send body separately ---
        char body_msg[4096];
        int body_len = snprintf(body_msg, sizeof(body_msg),
                                "Received POST data:\n%s", body_data);
        send(client_fd, body_msg, body_len, 0);
    }

    // --- Unsupported Methods ---
    else {
        char *resp = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        send(client_fd, resp, strlen(resp), 0);
    }

    close(client_fd);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Server running on http://localhost:%d\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&addr, &addrlen);
        if (client_fd < 0) { perror("accept"); continue; }
        handle_client(client_fd);
    }

    close(server_fd);
    return 0;
}
