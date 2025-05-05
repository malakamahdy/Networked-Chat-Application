#include "encryption.h"
#include "shared_key.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 9090               // The port it's connecting to (same as server)
#define BUFFER_SIZE 1024        // Max size for messages

// Function to print the welcome message and list of commands
void print_welcome_message() {
    printf("Welcome to the Networked Chat Application!\n");
    printf("Here are the list of commands available to users:\n\n");

    printf("1. Change Nickname:\n");
    printf("   /nick <new_nickname>\n");
    printf("   Example: /nick john\n\n");

    printf("2. List Online Users:\n");
    printf("   /list\n");
    printf("   Example: /list\n\n");

    printf("3. Send a Private Message:\n");
    printf("   /msg <nickname> <message>\n");
    printf("   Example: /msg alice Hello, Alice!\n\n");

    printf("4. React to a Message:\n");
    printf("   /react <nickname> <reaction>\n");
    printf("   Valid reactions: laugh, love, emphasize, question\n");
    printf("   Example: /react john laugh\n\n");

    printf("5. Create a Group Chat:\n");
    printf("   /group <nickname1>,<nickname2>,... <group_name> [message]\n");
    printf("   Example: /group alice,bob teamchat Hello team!\n\n");

    printf("6. Send a Message to a Group:\n");
    printf("   /msg group <group_name> <message>\n");
    printf("   Example: /msg group teamchat Let's plan the project!\n\n");

    printf("7. Quit the Application:\n");
    printf("   /quit\n");
    printf("   Example: /quit\n\n");
    
    printf("8. See available commands:\n");
    printf("   /help\n");
    printf("   Example: /help\n\n");
}

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

    // Print the welcome message and list of commands
    print_welcome_message();

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
        unsigned char encrypted[BUFFER_SIZE];
        // Encrypt the user's message using AES-256-CBC before sending to the server
        int encrypted_len = aes_encrypt((unsigned char *)message, strlen(message), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
        if (encrypted_len > 0) {
            send(sock, encrypted, encrypted_len, 0);
        }
    }

    // If we ever get here, clean up and close the socket
    close(sock);
    return 0;
}
