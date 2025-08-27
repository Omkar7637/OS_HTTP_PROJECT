// Header for input/output functions like printf() and perror()
#include <stdio.h>

// Header for utility functions like exit()
#include <stdlib.h>

// Header for memory operations like memset(), strlen()
#include <string.h>

// Header for POSIX system calls like close()
#include <unistd.h>

// Header for socket-related functions and constants like socket(), bind()
#include <sys/socket.h>

// Header for sockaddr_in structure and constants like INADDR_ANY
#include <netinet/in.h>

int main() {
    int sockfd;                          // Socket file descriptor
    struct sockaddr_in server_addr;      // Structure to hold server address information

    // 1. Create socket
    // socket(domain, type, protocol)
    // AF_INET      → IPv4
    // SOCK_STREAM  → TCP (connection-oriented)
    // 0            → Use default protocol for TCP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        // If socket creation fails, print error and exit
        perror("Socket creation failed");
        exit(1);
    }
    printf("Socket created successfully.\n");

    // 2. Set up the server address structure

    // sin_family specifies the address family: AF_INET for IPv4
    server_addr.sin_family = AF_INET;

    // sin_port specifies the port number (8080 in this case)
    // htons() converts host byte order to network byte order (big-endian)
    server_addr.sin_port = htons(8080);

    // sin_addr.s_addr specifies the IP address
    // INADDR_ANY allows the socket to accept connections from any IP address
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Zero the rest of the struct (optional but recommended for portability)
    // sin_zero is an unused field used to pad the struct to the size of struct sockaddr
    memset(&(server_addr.sin_zero), 0, 8);

    // 3. Bind the socket to the specified IP and port
    // bind() links the socket with the address info in server_addr
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        // If bind fails, print error, close socket, and exit
        perror("Bind failed");
        close(sockfd);
        exit(1);
    }

    printf("Bind successful. Socket is now linked to port 8080.\n");

    // 4. Close the socket after use
    close(sockfd);

    return 0;
}

/*
WHAT THIS CODE DOES:

- Creates a TCP socket using IPv4.
- Prepares a sockaddr_in structure to specify the port (8080) and IP (any).
- Binds the socket to the port and IP address.
- Closes the socket after successful binding.

WHY EACH HEADER IS INCLUDED:

- <stdio.h>        → printf(), perror()
- <stdlib.h>       → exit()
- <string.h>       → memset() to clear structure memory
- <unistd.h>       → close() to close socket
- <sys/socket.h>   → socket(), bind(), and socket constants
- <netinet/in.h>   → sockaddr_in, INADDR_ANY, htons()

POTENTIAL NEXT STEPS FOR A FULL SERVER:

1. listen()    → Mark the socket as passive to accept connections
2. accept()    → Accept an incoming connection
3. recv()/send() or read()/write() → Handle communication with the client
4. Implement loop or multithreading for handling multiple clients
5. close()     → Close each accepted socket and eventually the server socket

ADDITIONAL HEADERS FOR ADVANCED USAGE:

- <arpa/inet.h> → For inet_ntoa(), inet_pton(), converting IP addresses
- <errno.h>     → For advanced error handling and debugging
- <signal.h>    → To handle signals like SIGINT for clean shutdown

*/

