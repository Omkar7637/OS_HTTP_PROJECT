#define _GNU_SOURCE  // Enables GNU-specific functions like strcasestr()
#include <stdio.h>   // printf, perror, snprintf, sscanf
#include <stdlib.h>  // exit, malloc, free
#include <string.h>  // strstr, strlen, memcpy, etc.
#include <strings.h> // strncasecmp (case-insensitive compare)
#include <unistd.h>  // close(), read(), write()
#include <arpa/inet.h> // sockaddr_in, inet_ntoa, htons, etc. (networking)
#include <time.h>    // time(), ctime()
#include <pthread.h> // pthread_create, pthread_detach (for multithreading)

#define PORT 8080         // The TCP port number where the server listens
#define BUF_SIZE 4096     // Maximum buffer size for receiving headers+data
#define POST_MAX 4096     // Maximum size of POST body we accept/echo back

// =============================================================
// recv_all_headers()
// Reads from the client socket until HTTP headers are fully received
// (headers end with "\r\n\r\n"). It prints the raw request and how
// many times recv() was called.
// Returns: total number of bytes read into buf.
// =============================================================
int recv_all_headers(int client_fd, char *buf, size_t size) {
    int total = 0;      // total bytes read so far
    int n;              // number of bytes read in each recv()
    int recv_count = 0; // how many times recv() was called

    while ((n = recv(client_fd, buf + total, (int)(size - total - 1), 0)) > 0) {
        recv_count++;
        total += n;
        buf[total] = '\0'; // Null terminate to treat buffer as a string

        // Break if "\r\n\r\n" (end of headers) is found
        if (strstr(buf, "\r\n\r\n")) break;

        // Prevent buffer overflow (if request too big)
        if ((size_t)total >= size - 1) break;
    }

    printf("recv() was called %d times to read headers.\n", recv_count);
    printf("===== RAW HTTP REQUEST =====\n%s\n============================\n", buf);
    return total;
}

// =============================================================
// send_all()
// Safe wrapper for send() — ensures entire buffer is sent.
// send() may send fewer bytes than requested; this loop retries.
// =============================================================
static void send_all(int fd, const char *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, data + sent, len - sent, 0);
        if (n <= 0) break; // stop if error or connection closed
        sent += (size_t)n;
    }
}

// =============================================================
// handle_client()
// Handles an HTTP client connection (GET or POST).
// =============================================================
static void handle_client(int client_fd) {
    char reqbuf[BUF_SIZE];   // buffer for request (headers + maybe body)
    memset(reqbuf, 0, sizeof(reqbuf));

    // 1. Read all headers (and possibly some body if already sent)
    recv_all_headers(client_fd, reqbuf, sizeof(reqbuf));

    // 2. Parse request line: "METHOD PATH VERSION"
    char method[16] = {0}, path[256] = {0}, version[16] = {0};
    if (sscanf(reqbuf, "%15s %255s %15s", method, path, version) != 3) {
        // Malformed request → respond with 400 Bad Request
        const char *bad = "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n";
        send_all(client_fd, bad, strlen(bad));
        return;
    }

    // 3. Extract query string if present (?key=value&...)
    char *query = NULL;
    if (strchr(path, '?')) {
        char *qmark = strchr(path, '?');
        *qmark = '\0';   // terminate path at '?'
        query = qmark + 1; // query string starts after '?'
    }

    printf("Method=%s\nPath=%s\nVersion=%s\n", method, path, version);
    if (query) printf("Query=%s\n", query);

    // 4. Parse Host header
    char *host_hdr = strstr(reqbuf, "Host:");
    if (host_hdr) {
        host_hdr += 5; // skip "Host:"
        while (*host_hdr == ' ' || *host_hdr == '\t') host_hdr++; // skip spaces/tabs

        char host_val[256] = {0};
        size_t i = 0;
        // Copy until CRLF (end of line)
        while (*host_hdr && *host_hdr != '\r' && *host_hdr != '\n' && i < sizeof(host_val)-1) {
            host_val[i++] = *host_hdr++;
        }
        host_val[i] = '\0';
        printf("Host=%s\n", host_val);
    } else {
        printf("Host header not found!\n");
    }

    // 5. Reject unsupported HTTP methods (only GET & POST supported)
    if (strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0) {
        const char *mna =
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "405 - Method Not Allowed";
        send_all(client_fd, mna, strlen(mna));
        return;
    }

    // ==============================
    // Handle POST requests
    // ==============================
    if (strcmp(method, "POST") == 0) {
        int content_length = 0;

        // Scan headers for "Content-Length:"
        const char *scan = reqbuf;
        while (scan && *scan) {
            const char *line_end = strstr(scan, "\r\n");
            size_t line_len = line_end ? (size_t)(line_end - scan) : strlen(scan);

            if (line_len == 0) break; // reached CRLFCRLF (end of headers)

            if (strncasecmp(scan, "Content-Length:", 15) == 0) {
                sscanf(scan + 15, " %d", &content_length);
            }

            if (!line_end) break;
            scan = line_end + 2;
            if (line_end[2] == '\r' && line_end[3] == '\n') break;
        }

        if (content_length < 0) content_length = 0;
        if (content_length > POST_MAX) {
            printf("Warning: Content-Length=%d exceeds limit %d; truncating.\n",
                   content_length, POST_MAX);
            content_length = POST_MAX;
        }

        // Find start of body inside reqbuf (may already have part of body)
        char *body_start = strstr(reqbuf, "\r\n\r\n");
        int already_have = 0;
        char body_data[POST_MAX + 1];
        memset(body_data, 0, sizeof(body_data));

        if (body_start) {
            body_start += 4; // skip "\r\n\r\n"
            size_t avail = strlen(body_start);
            if ((int)avail > content_length) avail = (size_t)content_length;
            memcpy(body_data, body_start, avail);
            already_have = (int)avail;
        }

        // If we didn’t receive whole body, continue reading from socket
        int remaining = content_length - already_have;
        while (remaining > 0) {
            ssize_t n = recv(client_fd, body_data + already_have, (size_t)remaining, 0);
            if (n <= 0) break;
            already_have += (int)n;
            remaining -= (int)n;
        }
        body_data[already_have] = '\0';

        printf("===== POST BODY =====\n%s\n=====================\n", body_data);

        // Construct response headers (separate from body to avoid truncation issues)
        char header[256];
        size_t body_msg_len = 20 + strlen(body_data); // "Received POST data:\n" = 20 chars
        int header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %zu\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n",
            body_msg_len);

        send_all(client_fd, header, (size_t)header_len);

        // Construct and send response body
        char body_msg[POST_MAX + 64];
        int payload_len = snprintf(body_msg, sizeof(body_msg),
                                   "Received POST data:\n%s", body_data);
        if (payload_len < 0) payload_len = 0;
        send_all(client_fd, body_msg, (size_t)payload_len);
        return; // finished POST
    }

    // ==============================
    // Handle GET requests
    // ==============================
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

    // Build and send HTTP response for GET
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

// =============================================================
// thread_func()
// Thread wrapper that runs handle_client() for each client.
// Frees allocated memory and closes socket when done.
// =============================================================
static void *thread_func(void *arg) {
    int client_fd = *(int*)arg;
    free(arg); // free memory allocated in main()
    handle_client(client_fd);
    close(client_fd); // close socket after handling
    return NULL;
}

// =============================================================
// main()
// Entry point: Creates socket, binds, listens, and accepts clients.
// Each client connection is handled in a new detached thread.
// =============================================================
int main(void) {
    int server_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    // 1. Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    // 2. Allow reuse of address:port (avoid "Address already in use" on restart)
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. Bind to IP:PORT
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // accept connections on any interface
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 4. Start listening (16 pending connections max in queue)
    if (listen(server_fd, 16) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server running on http://localhost:%d\n", PORT);

    // 5. Accept loop — each client handled in a new thread
    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr*)&addr, &addrlen);
        if (client_fd < 0) { perror("accept"); continue; }

        // Allocate client_fd dynamically (so thread has its own copy)
        int *pclient = (int*)malloc(sizeof(int));
        if (!pclient) { close(client_fd); continue; }
        *pclient = client_fd;

        pthread_t tid;
        // Create thread to handle client
        if (pthread_create(&tid, NULL, thread_func, pclient) == 0) {
            pthread_detach(tid); // auto-clean resources when thread ends
        } else {
            perror("pthread_create");
            close(client_fd);
            free(pclient);
        }
    }

    close(server_fd);
    return 0;
}
