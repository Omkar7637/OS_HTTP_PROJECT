// Header for standard I/O functions like printf(), perror()
#include <stdio.h>

// Header for general utilities like exit()
#include <stdlib.h>

// Header for string operations like memset(), strlen()
#include <string.h>

// Header for POSIX system calls like close()
#include <unistd.h>

// Header for socket functions like socket(), bind(), listen(), accept(), recv(), send()
#include <sys/socket.h>

// Header for Internet address structures like sockaddr_in, INADDR_ANY
#include <netinet/in.h>

// Header for converting IP addresses (e.g., inet_ntop())
#include <arpa/inet.h>

int main() {
    int server_fd, client_fd;                      // Socket file descriptors
    struct sockaddr_in server_addr, client_addr;   // Server and client address structures
    socklen_t addr_size;                           // Size of the client address structure
    char buffer[1024];                             // Buffer to hold incoming messages
    char client_ip[INET_ADDRSTRLEN];               // To store client IP address in string format

    // 1. Create a TCP socket using IPv4
    // Parameters: AF_INET = IPv4, SOCK_STREAM = TCP, 0 = default protocol
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // 2. Setup server address
    server_addr.sin_family = AF_INET;              // Use IPv4
    server_addr.sin_port = htons(8080);            // Port 8080 (convert to network byte order)
    server_addr.sin_addr.s_addr = INADDR_ANY;      // Accept connections on any IP address
    memset(&(server_addr.sin_zero), 0, 8);         // Clear remaining fields for portability

    // Bind the socket to the specified IP and port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }

    // 3. Start listening for incoming connections
    // The second argument is the backlog size (queue limit)
    listen(server_fd, 5);
    printf("Server listening on port 8080...\n");

    // 4. Accept an incoming client connection
    addr_size = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
    if (client_fd < 0) {
        perror("Accept failed");
        close(server_fd);
        exit(1);
    }

    // Convert client IP to human-readable string and display it
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    printf("Client connected from %s:%d\n", client_ip, ntohs(client_addr.sin_port));

    // 5. Communication: Receive message from client and respond
    memset(buffer, 0, sizeof(buffer));  // Clear buffer before receiving data

    // Receive message from the client
    recv(client_fd, buffer, sizeof(buffer), 0);
    printf("Client says: %s\n", buffer);

    // Send a response back to the client
    char *reply = "Hello from server!";
    send(client_fd, reply, strlen(reply), 0);

    // 6. Close both sockets
    close(client_fd);
    close(server_fd);

    return 0;
}

/*
WHAT THIS PROGRAM DOES:

1. Creates a TCP server socket that listens on port 8080.
2. Binds to all available IP addresses (INADDR_ANY).
3. Accepts a single client connection.
4. Receives a message from the client.
5. Sends a response back to the client.
6. Closes the client and server sockets.

EXPLANATION OF INCLUDED HEADERS:

- <stdio.h>        → For printf(), perror()
- <stdlib.h>       → For exit()
- <string.h>       → For memset(), strlen()
- <unistd.h>       → For close()
- <sys/socket.h>   → For socket(), bind(), listen(), accept(), recv(), send()
- <netinet/in.h>   → For sockaddr_in, htons(), INADDR_ANY
- <arpa/inet.h>    → For inet_ntop(), to convert binary IP to string

FUNCTION EXPLANATIONS:

- socket()     → Creates a socket and returns a file descriptor
- bind()       → Binds the socket to an IP address and port
- listen()     → Prepares the socket to accept incoming connections
- accept()     → Accepts an incoming connection and returns a new socket descriptor
- recv()       → Receives data from the client
- send()       → Sends data to the client
- close()      → Closes the socket to release system resources
- inet_ntop()  → Converts the client's IP from binary to readable format
- ntohs()      → Converts client's port to host byte order for display

THINGS TO ADD FOR A ROBUST SERVER:

- Loop to accept multiple clients
- Use threads or fork() to handle each client concurrently
- Add error checking for recv() and send()
- Handle client disconnections gracefully
- Add timeout or select()/poll() for multiple socket handling
*/
