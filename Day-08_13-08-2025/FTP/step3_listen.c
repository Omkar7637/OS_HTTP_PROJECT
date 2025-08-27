// Header for standard input/output functions like printf(), perror()
#include <stdio.h>

// Header for utility functions like exit()
#include <stdlib.h>

// Header for memory operations like memset(), strlen()
#include <string.h>

// Header for POSIX system calls like close()
#include <unistd.h>

// Header for socket-related functions like socket(), bind(), listen()
#include <sys/socket.h>

// Header for internet address structures and constants like sockaddr_in, INADDR_ANY
#include <netinet/in.h>

int main() {
    int sockfd;                          // Socket file descriptor
    struct sockaddr_in server_addr;      // Struct to hold server address info

    // 1. Create a socket
    // socket(domain, type, protocol)
    // domain: AF_INET       → IPv4
    // type:   SOCK_STREAM   → TCP (connection-oriented)
    // proto:  0             → Use default protocol (TCP)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        // If socket creation fails, print error and exit
        perror("Socket creation failed");
        exit(1);
    }
    printf("Socket created successfully.\n");

    // 2. Setup the server address structure
    server_addr.sin_family = AF_INET;             // Address family: IPv4
    server_addr.sin_port = htons(8080);           // Port number (host to network byte order)
    server_addr.sin_addr.s_addr = INADDR_ANY;     // Accept connections from any IP address
    memset(&(server_addr.sin_zero), 0, 8);        // Zero the remaining struct padding

    // 3. Bind the socket to the IP/port
    // bind() links the socket to the address structure
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(1);
    }
    printf("Bind successful.\n");

    // 4. Start listening for incoming connections
    // listen(sockfd, backlog)
    // backlog: number of pending connections that can be queued (here, 5)
    if (listen(sockfd, 5) < 0) {
        perror("Listen failed");
        close(sockfd);
        exit(1);
    }
    printf("Server is now listening on port 8080...\n");

    // Note: We're not accepting client connections in this example
    // For a full server, you'd call accept() here in a loop

    // 5. Close the socket after use
    close(sockfd);
    return 0;
}

/*
WHAT THIS CODE DOES:

- Creates a TCP socket using IPv4 (AF_INET, SOCK_STREAM)
- Sets up a sockaddr_in structure with port 8080 and any IP
- Binds the socket to the port and IP
- Starts listening for incoming TCP connections
- Closes the socket without accepting any client

WHY EACH HEADER IS INCLUDED:

- <stdio.h>        → For printf(), perror()
- <stdlib.h>       → For exit()
- <string.h>       → For memset() to zero the sockaddr_in struct
- <unistd.h>       → For close() to close socket descriptor
- <sys/socket.h>   → For socket(), bind(), listen(), and socket options
- <netinet/in.h>   → For sockaddr_in, htons(), INADDR_ANY

WHAT CAN BE ADDED NEXT (FOR A FULL TCP SERVER):

1. accept()       → Accept incoming client connections
2. read()/recv()  → Receive data from clients
3. write()/send() → Send data to clients
4. loop()         → Run accept in a loop to handle multiple clients
5. fork()/threads → Handle multiple clients concurrently
6. signal handling → Clean shutdown using signals like SIGINT

ADDITIONAL HEADERS (OPTIONAL):

- <arpa/inet.h>   → For inet_pton(), inet_ntoa() if using IP addresses as strings
- <errno.h>       → For accessing error numbers (used with perror or strerror)
- <sys/types.h>   → Sometimes used for socket types on some systems

NOTES:

- `htons()` converts the port number to network byte order (big-endian)
- `INADDR_ANY` means the server can receive connections on any of its IP addresses
- `sin_zero` is just padding to make `sockaddr_in` the same size as `sockaddr`
*/
