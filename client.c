// client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 9090               // The port it's connecting to (same as server)
#define BUFFER_SIZE 1024        // Max size for messages

// Function runs on a separate thread and handles incoming messages from the server
void *receive_handler(void *socket_desc) 
{
    int sock = *(int *)socket_desc;        // Casts the argument back to an int socket
    char buffer[BUFFER_SIZE];
    int read_size;

    // Keep listening for messages from the server
    while ((read_size = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        buffer[read_size] = '\0';          // Null-terminating the received message
        printf("%s", buffer);              // Printing the message to stdout
    }

    // NOTE: If the loop exits, the server probably disconnected
    return 0;
}

int main() 
{
    int sock;
    struct sockaddr_in server;
    char message[BUFFER_SIZE];
    pthread_t recv_thread;

    // Create a TCP socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // Define server properties (localhost, port 9090)
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP (localhost)
    server.sin_family = AF_INET;                     // IPv4
    server.sin_port = htons(PORT);                   // Convert port to network byte order

    // Connect to the server
    connect(sock, (struct sockaddr *)&server, sizeof(server));

    // Create a separate thread to receive messages so we can send and receive at the same time
    pthread_create(&recv_thread, NULL, receive_handler, &sock);

    // Main thread keeps reading user input and sending it to the server
    while (fgets(message, sizeof(message), stdin) != NULL) {
        send(sock, message, strlen(message), 0);
    }

    // If we ever get here, clean up and close the socket
    close(sock);
    return 0;
}