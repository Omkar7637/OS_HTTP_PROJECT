// Standard I/O functions like printf(), perror()
#include <stdio.h>

// For functions like exit()
#include <stdlib.h>

// For socket(), bind(), listen(), accept() functions and socket types (SOCK_STREAM etc.)
#include <sys/socket.h>

// For sockaddr_in structure, AF_INET, htons(), INADDR_ANY
#include <netinet/in.h>

// For close() to properly close sockets
#include <unistd.h>

int main() 
{
    int sockfd; // This will hold the file descriptor (an integer ID) for the socket

    // Create a socket using IPv4 (AF_INET), TCP (SOCK_STREAM), and default protocol (0)
    // Returns a socket file descriptor (sockfd) if successful, or -1 on failure
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Check if socket creation failed
    if (sockfd < 0) 
    {
        // perror prints a descriptive error message
        perror("Socket creation failed");

        // Exit the program with status 1 (error)
        exit(1);
    }

    // If socket creation succeeded, print its file descriptor
    printf("Socket created successfully! sockfd = %d\n", sockfd);

    // Close the socket to free resources
    close(sockfd);
    
    // Return 0 to indicate successful execution
    return(0);
}

/*
WHAT THIS CODE DOES:

- Creates a TCP socket using IPv4 (AF_INET + SOCK_STREAM)
- Prints the socket file descriptor
- Closes the socket
- Does NOT bind, listen, or accept connections (not a complete server)

WHY EACH HEADER IS INCLUDED:

- <stdio.h>      → For printf() and perror()
- <stdlib.h>     → For exit()
- <sys/socket.h> → For socket-related system calls
- <netinet/in.h> → For sockaddr_in, AF_INET, etc.
- <unistd.h>     → For close()

WHAT ELSE CAN BE ADDED (FOR A FULL SERVER):

1. bind()     → Attach socket to a specific IP and port
2. listen()   → Start listening for client connections
3. accept()   → Accept incoming client requests
4. read()/write() or recv()/send() → For communication
5. Optionally use fork()/threads for handling multiple clients

For a TCP Client Program:
1. socket()
2. connect()
3. read()/write()
4. close()

ADDITIONAL HEADERS THAT MAY BE USED LATER:

- <string.h>    → For memset(), strlen(), etc.
- <arpa/inet.h> → For inet_addr(), inet_ntoa(), inet_pton()
- <errno.h>     → For advanced error handling

*/
