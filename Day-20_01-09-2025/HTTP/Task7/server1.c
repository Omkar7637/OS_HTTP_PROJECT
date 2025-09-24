#define _GNU_SOURCE  // for strcasestr on GNU libc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>   // strncasecmp
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define PORT 8080
#define BUF_SIZE 4096
#define POST_MAX 4096   // max bytes we'll store/echo for POST body

// ---------------- recv_all_headers() ----------------
// Reads until end-of-headers (\r\n\r\n), prints the raw request and recv() call count.
// Returns total bytes currently in buf (may include some of the body if it arrived together).
int recv_all_headers(int client_fd, char *buf, size_t size) {
    int total = 0, n, recv_count = 0;
    while ((n = recv(client_fd, buf + total, (int)(size - total - 1), 0)) > 0) {
        recv_count++;
        total += n;
        buf[total] = '\0';
        if (strstr(buf, "\r\n\r\n")) break; // end of headers seen
        if ((size_t)total >= size - 1) break; // buffer full (avoid overflow)
    }
    printf("recv() was called %d times to read headers.\n", recv_count);
    printf("===== RAW HTTP REQUEST =====\n%s\n============================\n", buf);
    return total;
}

// Safely send a buffer (handle partial sends)
static void send_all(int fd, const char *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, data + sent, len - sent, 0);
        if (n <= 0) break;
        sent += (size_t)n;
    }
}

static void handle_client(int client_fd) {
    char reqbuf[BUF_SIZE];
    memset(reqbuf, 0, sizeof(reqbuf));
    recv_all_headers(client_fd, reqbuf, sizeof(reqbuf));

    // ---- Parse request line: METHOD PATH VERSION
    char method[16] = {0}, path[256] = {0}, version[16] = {0};
    if (sscanf(reqbuf, "%15s %255s %15s", method, path, version) != 3) {
        const char *bad = "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n";
        send_all(client_fd, bad, strlen(bad));
        return;
    }

    // ---- Extract & print query (Task 5)
    char *query = NULL;
    if (strchr(path, '?')) {
        char *qmark = strchr(path, '?');
        *qmark = '\0';
        query = qmark + 1;
    }

    printf("Method=%s\nPath=%s\nVersion=%s\n", method, path, version);
    if (query) printf("Query=%s\n", query);

    // ---- Parse & print Host header (Task 3)
    char *host_hdr = strstr(reqbuf, "Host:");
    if (host_hdr) {
        host_hdr += 5; // skip "Host:"
        while (*host_hdr == ' ' || *host_hdr == '\t') host_hdr++;
        char host_val[256] = {0};
        size_t i = 0;
        while (*host_hdr && *host_hdr != '\r' && *host_hdr != '\n' && i < sizeof(host_val)-1) {
            host_val[i++] = *host_hdr++;
        }
        host_val[i] = '\0';
        printf("Host=%s\n", host_val);
    } else {
        printf("Host header not found!\n");
    }

    // ---- If method is not GET or POST, return 405 (Task 2 requirement)
    if (strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0) {
        const char *mna =
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "405 - Method Not Allowed";
        send_all(client_fd, mna, strlen(mna));
        return;
    }

    // ---- If POST, read Content-Length and body (Task 6)
    if (strcmp(method, "POST") == 0) {
        // Case-insensitive search for Content-Length
        int content_length = 0;
        // Robustly scan header lines for "Content-Length:"
        const char *scan = reqbuf;
        while (scan && *scan) {
            const char *line_end = strstr(scan, "\r\n");
            size_t line_len = line_end ? (size_t)(line_end - scan) : strlen(scan);
            if (line_len == 0) break; // reached CRLFCRLF
            if (strncasecmp(scan, "Content-Length:", 15) == 0) {
                sscanf(scan + 15, " %d", &content_length);
            }
            if (!line_end) break;
            scan = line_end + 2;
            if (line_end[2] == '\r' && line_end[3] == '\n') break; // end-of-headers safety
        }

        if (content_length < 0) content_length = 0;
        if (content_length > POST_MAX) {
            // Clamp to our buffer limit; read & print only first POST_MAX bytes
            printf("Warning: Content-Length=%d exceeds limit %d; truncating.\n",
                   content_length, POST_MAX);
            content_length = POST_MAX;
        }

        // Find start of body within reqbuf (could already contain some/all body)
        char *body_start = strstr(reqbuf, "\r\n\r\n");
        int already_have = 0;
        char body_data[POST_MAX + 1];
        memset(body_data, 0, sizeof(body_data));

        if (body_start) {
            body_start += 4; // skip CRLFCRLF
            // Copy up to content_length or remaining buffer
            size_t avail = strlen(body_start);
            if ((int)avail > content_length) avail = (size_t)content_length;
            memcpy(body_data, body_start, avail);
            already_have = (int)avail;
        }

        // Read the remaining body bytes (if any)
        int remaining = content_length - already_have;
        while (remaining > 0) {
            ssize_t n = recv(client_fd, body_data + already_have, (size_t)remaining, 0);
            if (n <= 0) break;
            already_have += (int)n;
            remaining -= (int)n;
        }
        body_data[already_have] = '\0';

        printf("===== POST BODY =====\n%s\n=====================\n", body_data);

        // Build header only, then send body separately (prevents snprintf truncation warnings)
        char header[256];
        // "Received POST data:\n" is 20 bytes
        size_t body_msg_len = 20 + strlen(body_data);
        int header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %zu\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n",
            body_msg_len);

        send_all(client_fd, header, (size_t)header_len);

        char body_msg[POST_MAX + 64]; // enough for prefix + body
        int payload_len = snprintf(body_msg, sizeof(body_msg),
                                   "Received POST data:\n%s", body_data);
        if (payload_len < 0) payload_len = 0;
        send_all(client_fd, body_msg, (size_t)payload_len);
        return;
    }

    // ---- GET routing (Task 4)
    char body[1024];
    if (strcmp(path, "/hello") == 0) {
        snprintf(body, sizeof(body), "Hello Student!");
    } else if (strcmp(path, "/time") == 0) {
        time_t now = time(NULL);
        // ctime() includes a newline; that's okay for plain text
        snprintf(body, sizeof(body), "Current Time: %s", ctime(&now));
    } else {
        snprintf(body, sizeof(body), "Unknown Path!");
        if (query) {
            strncat(body, "\nQuery=", sizeof(body) - strlen(body) - 1);
            strncat(body, query, sizeof(body) - strlen(body) - 1);
        }
    }

    // Send GET response (simple single buffer is fine here)
    char resp[2048];
    int resp_len = snprintf(resp, sizeof(resp),
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: %zu\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        strlen(body), body);
    if (resp_len < 0) resp_len = 0;
    send_all(client_fd, resp, (size_t)resp_len);
}

// ---------------- Thread wrapper (Task 7) ----------------
static void *thread_func(void *arg) {
    int client_fd = *(int*)arg;
    free(arg);
    handle_client(client_fd);
    close(client_fd);
    return NULL;
}

int main(void) {
    int server_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 16) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server running on http://localhost:%d\n", PORT);

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr*)&addr, &addrlen);
        if (client_fd < 0) { perror("accept"); continue; }

        // Spawn a detached thread per client (Task 7)
        int *pclient = (int*)malloc(sizeof(int));
        if (!pclient) { close(client_fd); continue; }
        *pclient = client_fd;

        pthread_t tid;
        if (pthread_create(&tid, NULL, thread_func, pclient) == 0) {
            pthread_detach(tid); // auto-clean resources when thread returns
        } else {
            perror("pthread_create");
            close(client_fd);
            free(pclient);
        }
    }

    close(server_fd);
    return 0;
}
