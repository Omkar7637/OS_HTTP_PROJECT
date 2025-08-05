#include <stdio.h>      // For printf(), perror()
#include <stdlib.h>     // For exit()
#include <sys/socket.h> // For socket(), AF_INET, SOCK_STREAM
#include <netinet/in.h> // For sockaddr_in, INADDR_ANY, htons()
#include <unistd.h>     // For close()


int main() 
{
    int sockfd;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0) 
    {
        perror("Socket creation failed");
        exit(1);
    }

    printf("Socket created successfully! sockfd = %d\n", sockfd);

    // Close the socket
    close(sockfd);
    
    return(0);
}
