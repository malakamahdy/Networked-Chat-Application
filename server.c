// server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 9090               // Port number the server will listen on
#define MAX_CLIENTS 10          // Max number of clients that can connect
#define BUFFER_SIZE 1024        // Max size for messages

int clients[MAX_CLIENTS];       // Array to keep track of connected clients
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // Mutex to avoid race conditions

// Thread function to handle communication with a single client
void *handle_client(void *arg) 
{
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int read_size;

    // Keep reading messages from this client
    while ((read_size = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[read_size] = '\0'; // Null-terminate the message

        pthread_mutex_lock(&lock); // Lock before accessing shared client list

        // Loop through all clients and send this message to everyone except the sender
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] && clients[i] != client_socket) {
                send(clients[i], buffer, strlen(buffer), 0);
            }
        }

        pthread_mutex_unlock(&lock); // Unlock after we're done broadcasting
    }

    // If we get here, client disconnected or something went wrong
    close(client_socket);

    // Remove client from the list
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client_socket) {
            clients[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL); // End the thread
}

int main() {
    int server_socket, client_socket, c;
    struct sockaddr_in server, client;
    pthread_t tid;

    // Create a TCP socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Set up server address structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP
    server.sin_port = htons(PORT);       // Host to network short (convert port to right byte order)

    // Bind the socket to the port
    bind(server_socket, (struct sockaddr *)&server, sizeof(server));

    // Start listening for incoming connections
    listen(server_socket, MAX_CLIENTS);
    printf("Server started on port %d\n", PORT);

    // Accept incoming client connections in a loop
    while ((client_socket = accept(server_socket, (struct sockaddr *)&client, (socklen_t*)&c))) {
        pthread_mutex_lock(&lock);

        // Find a free spot in the clients array
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i]) {
                clients[i] = client_socket;

                // Create a new thread to handle this client
                pthread_create(&tid, NULL, handle_client, &clients[i]);
                break;
            }
        }

        pthread_mutex_unlock(&lock);
    }

    // Close server socket when done (never really happens in this case)
    close(server_socket);
    return 0;
}
