# OS_HTTP_PROJECT
Here's a **step-by-step, parameter-by-parameter explanation** of the given **simple HTTP server code in C**, with **terminology**, **purpose**, and **what else you can use in each case**:

---

## HEADER FILES AND WHY USED

```c
#include <stdio.h>       // printf(), perror(), snprintf()
#include <stdlib.h>      // exit(), EXIT_FAILURE
#include <string.h>      // memset(), strcmp(), sscanf()
#include <unistd.h>      // close(), read(), write()
#include <arpa/inet.h>   // inet_ntop(), htons(), INET_ADDRSTRLEN
#include <signal.h>      // signal(), SIGINT
#include <sys/socket.h>  // socket(), bind(), listen(), accept()
#include <netinet/in.h>  // sockaddr_in
#include <time.h>        // time(), localtime(), strftime()
```

| Header         | Used For                                                                |
| -------------- | ----------------------------------------------------------------------- |
| `stdio.h`      | Input/output functions like `printf`, `perror`, `snprintf`              |
| `stdlib.h`     | Exit control (`exit`, `EXIT_FAILURE`)                                   |
| `string.h`     | String functions like `memset`, `strcmp`, `sscanf`                      |
| `unistd.h`     | Unix stdlib: `close`, `read`, `write`                                   |
| `arpa/inet.h`  | Address manipulation: `inet_ntop`, `htons`, etc.                        |
| `signal.h`     | Handle `SIGINT` (Ctrl+C) using `signal()`                               |
| `sys/socket.h` | Core socket functions: `socket`, `bind`, `listen`, `accept`             |
| `netinet/in.h` | Definitions for `sockaddr_in` struct (used for IP address/port binding) |
| `time.h`       | Time/date operations                                                    |

---

## GLOBAL DEFINITIONS

```c
#define PORT 8080
int server_fd;
```

* `PORT`: The port number server listens on.

  * Other valid values: Any port `1024–65535` (Avoid 0–1023 which are privileged).
  * Example: `#define PORT 80` for standard HTTP.

* `server_fd`: File descriptor for the **server socket**, used across the code.

---

## SIGNAL HANDLER

```c
void handle_sigint(int sig) {
    printf("\nShutting down server...\n");
    close(server_fd);
    exit(0);
}
```

* **Purpose**: When Ctrl+C is pressed, this function gets called to gracefully close the server.
* `int sig`: The signal number (here, `SIGINT`).
* `close(server_fd)`: Frees the port.
* `exit(0)`: Exit the program cleanly.

---

## MAIN FUNCTION LOGIC

### 1. Create TCP socket

```c
server_fd = socket(AF_INET, SOCK_STREAM, 0);
```

| Parameter     | Meaning                            | Alternatives                     |
| ------------- | ---------------------------------- | -------------------------------- |
| `AF_INET`     | IPv4 Address family                | `AF_INET6` for IPv6              |
| `SOCK_STREAM` | TCP socket                         | `SOCK_DGRAM` for UDP             |
| `0`           | Protocol (0 = auto-select for TCP) | Can use `IPPROTO_TCP` explicitly |

---

### 2. Set socket options

```c
int opt = 1;
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

| Parameter             | Meaning                                                  |
| --------------------- | -------------------------------------------------------- |
| `SOL_SOCKET`          | Set options at the socket level                          |
| `SO_REUSEADDR`        | Allows immediate reuse of the port after program restart |
| `&opt`, `sizeof(opt)` | Pointer to value and its size (must be `int`)            |

You can also use:

* `SO_REUSEPORT`: Allows multiple sockets to bind to same port
* `SO_KEEPALIVE`: Detect broken connections

---

### 3. Configure server address

```c
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(PORT);
server_addr.sin_addr.s_addr = INADDR_ANY;
memset(&(server_addr.sin_zero), 0, 8);
```

| Field                    | Purpose                                                 |
| ------------------------ | ------------------------------------------------------- |
| `sin_family = AF_INET`   | IPv4                                                    |
| `sin_port = htons(PORT)` | Convert host byte order to network byte order (`htons`) |
| `s_addr = INADDR_ANY`    | Accept connection on **all interfaces** (0.0.0.0)       |
| `sin_zero`               | Padding to match struct size                            |

---

### 4. Bind the socket

```c
bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))
```

* `bind()` links socket with IP+Port.
* Cast to `(struct sockaddr*)` as `bind()` expects a generic `sockaddr`.

---

### 5. Listen for incoming connections

```c
listen(server_fd, 5);
```

* `5` is the **backlog** — max number of pending connections in queue.
* You can increase it if expecting more load.

---

### SERVER LOOP

```c
while (1) {
```

Main loop runs infinitely, accepting clients.

---

### 6. Accept client connection

```c
client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
```

* Blocks until a new connection.
* `client_fd`: New socket for communication with the client.
* `client_addr`: Contains client’s IP and port.

---

### 7. Convert client IP to string

```c
inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
```

* Converts binary IP to human-readable string (`xxx.xxx.xxx.xxx`)
* `INET_ADDRSTRLEN` = 16, enough for IPv4.

---

### 8. Read client’s request

```c
ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
```

* Reads raw HTTP request into `buffer`
* `memset()` used before to clear the buffer
* `bytes_read > 0`: ensures data is received

---

### 9. Log full raw HTTP request

```c
printf("===== RAW HTTP REQUEST =====\n%s\n", buffer);
```

---

### 10. Parse method and path

```c
sscanf(buffer, "%s %s", method, path);
```

* `sscanf()` reads the request line (`GET /hello HTTP/1.1`)
* `method`: `"GET"`, `"POST"` etc.
* `path`: `"/hello"` or `"/bye"` or others

---

### 11. Get current date/time

```c
time_t now = time(NULL);
struct tm *t = localtime(&now);
strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", t);
```

* `time(NULL)`: Get current time
* `localtime()`: Convert to local format
* `strftime()`: Format into readable string

---

### 12. Prepare HTTP response header

```c
const char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
```

* Minimal valid HTTP header
* You can add:

  * `Content-Length`
  * `Connection: close`
  * `Cache-Control`

---

### 13. Form dynamic HTML content

```c
snprintf(response, sizeof(response),
         "%s<html><body><h1>...</h1><p>...</p></body></html>",
         header, client_ip, date_str);
```

* Depending on path (`/hello`, `/bye`, others)
* Uses `client_ip` and `date_str` dynamically

---

### 14. Send response to client

```c
write(client_fd, response, strlen(response));
```

* Writes HTTP response to client
* You can also use `send()` instead of `write()`

---

### 15. Close client connection

```c
close(client_fd);
```

* Done after serving one request.
* For keep-alive connections, you'd keep this open.

---

## WHAT YOU CAN ADD / IMPROVE

| Improvement                 | How/Why                                                              |
| --------------------------- | -------------------------------------------------------------------- |
| Handle more methods         | Add support for `POST`, `PUT`, etc.                                  |
| Serve static files          | Use `fopen()` and `fread()` to serve `.html`, `.css` files           |
| Add `Content-Length` header | For better client compatibility                                      |
| Multithreading/forking      | Handle multiple clients concurrently                                 |
| Log to file                 | Log requests to a file for monitoring                                |
| HTTPS support               | Use `OpenSSL` to secure connections                                  |
| Better error handling       | Respond with `404`, `500`, etc. based on request and internal errors |

---

## SUMMARY

This code is a **single-threaded HTTP/1.1 server** that:

* Listens on port 8080
* Accepts only `GET` requests
* Responds with dynamic HTML based on path (`/hello`, `/bye`)
* Gracefully handles Ctrl+C (SIGINT)
