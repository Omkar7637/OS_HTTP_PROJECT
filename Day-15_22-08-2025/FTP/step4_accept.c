// Header for standard input/output functions like printf(), perror()
#include <stdio.h>

// Header for utility functions like exit()
#include <stdlib.h>

// Header for string manipulation functions like memset()
#include <string.h>

// Header for POSIX system calls like close()
#include <unistd.h>

// Header for socket programming functions like socket(), bind(), listen(), accept()
#include <sys/socket.h>

// Header for internet address structures and constants
#include <netinet/in.h>

// Header required for IP address conversions like inet_ntop()
#include <arpa/inet.h>

int main() {
    int server_fd, client_fd;                      // File descriptors for server and client sockets
    struct sockaddr_in server_addr, client_addr;   // Structures for server and client addresses
    socklen_t addr_size;                           // To store size of client address
    char client_ip[INET_ADDRSTRLEN];               // To store client IP address in string format

    // 1. Create a TCP socket using IPv4
    // socket(domain, type, protocol)
    // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = default protocol
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    printf("Socket created.\n");

    // 2. Setup server address structure
    server_addr.sin_family = AF_INET;              // IPv4
    server_addr.sin_port = htons(8080);            // Port 8080 (converted to network byte order)
    server_addr.sin_addr.s_addr = INADDR_ANY;      // Accept connections from any IP
    memset(&(server_addr.sin_zero), 0, 8);         // Zero out the rest of the struct

    // Bind the socket to the IP/Port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }
    printf("Bind successful.\n");

    // 3. Start listening for client connections
    // Second argument is the backlog (max queue size for pending connections)
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(1);
    }
    printf("Server is listening on port 8080...\n");

    // 4. Accept a client connection
    // This blocks until a client connects
    addr_size = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
    if (client_fd < 0) {
        perror("Accept failed");
        close(server_fd);
        exit(1);
    }

    // Optional: display the IP and port of the connected client
    // inet_ntop() converts binary IP to string format
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    printf("Client connected from %s:%d\n", client_ip, ntohs(client_addr.sin_port));

    // 5. Close both client and server sockets
    close(client_fd);
    close(server_fd);

    return 0;
}

/*
WHAT THIS CODE DOES:

1. Creates a TCP socket using IPv4.
2. Configures the socket to listen on port 8080.
3. Binds the socket to all available network interfaces (INADDR_ANY).
4. Starts listening for incoming TCP connections.
5. Accepts the first client connection and prints the client's IP and port.
6. Closes the client and server sockets.

HEADER EXPLANATION:

- <stdio.h>        → printf(), perror()
- <stdlib.h>       → exit()
- <string.h>       → memset()
- <unistd.h>       → close()
- <sys/socket.h>   → socket(), bind(), listen(), accept()
- <netinet/in.h>   → sockaddr_in, htons(), INADDR_ANY
- <arpa/inet.h>    → inet_ntop(), converts binary IP to readable string

KEY FUNCTIONAL DETAILS:

- `socket()` creates a socket and returns a file descriptor.
- `bind()` attaches the socket to a specific IP address and port.
- `listen()` marks the socket as passive, ready to accept connections.
- `accept()` waits and returns a new socket descriptor for client communication.
- `inet_ntop()` is used to convert the client’s binary IP to a readable string.
- `ntohs()` converts port number from network byte order to host byte order.

TO EXTEND THIS PROGRAM:

- Use `read()` or `recv()` to receive data from the client.
- Use `write()` or `send()` to send responses back.
- Place `accept()` inside a loop to handle multiple clients.
- Use `fork()` or threads to handle multiple clients concurrently.
- Add error logging, signal handling (e.g. for Ctrl+C), and timeouts for robustness.

*/
