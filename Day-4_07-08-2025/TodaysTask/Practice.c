#include<stdio.h>
#include<stdlib.h> // exit()
#include<sys/socket.h> // socket() AF_INET, SOCK_STREAM, bind()
#include<string.h> // memset()
#include<netinet/in.h> // sockaddr_in, server_addr
#include<unistd.h> // close()

int main()
{
    // Variable declarations
    int server_fd; // File decscripter for socket
    int client_fd; // File descriptor for client socket
    struct sockaddr_in server_addr; // Structure to hold server address info
    struct sockaddr_in client_addr; // Structure to hold client address info
    socklen_t addr_size; // To store size of address
    char client_ip[INET_ADDRSTRLEN]; // Buffer to hold client IP address


    // Code

    // Step 1: Create a TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0)
    {
        perror("Socket Creation Failed!");
        exit(1);
    }
    else
    {
        printf("Socket Created Successfully!\n");
    }

    // Step 2: Set up server address structure
    server_addr.sin_family = AF_INET; // IPV4
    server_addr.sin_port = htons(8080); // Port 8080 in network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accept connection on any local IP Address
    memset(&(server_addr.sin_zero), 0, 8); // Zero the rest of the structure

    // Step 3: Bind the socket to the Specified IP and Port
    if( bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 )
    {
        perror("Bind Failed!");
        close(server_fd); // Clean up if bind fails
        exit(1);
    }
    else
    {
        printf("Socket Bind Successfully!\n");
    }

    // 4. Listen for incoming connections
    if(listen(server_fd, 5) < 0) // 5 is the backlog for pending connections
    {
        perror("Listen Failed!");
        close(server_fd);
        exit(1);
    }
    else
    {
        printf("Server Listening on Port 8080...\n");
    }

    // 5. Accept incoming connection from client
    addr_size = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
    if(client_fd < 0)
    {
        perror("Accept Failed!");
        close(server_fd);
        exit(1);
    }
    else
    {
        printf("Client Connected Successfully!\n");
    }

    // Convert client IP address to human-readable string



    // Cleanup
    close(server_fd); // Close the socket
    printf("Socket Closed Successfully!\n");
    return(0);
}