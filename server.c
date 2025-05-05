#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "commands.h"

#define PORT 9090               // Port number the server will listen on
#define MAX_CLIENTS 10          // Max number of clients that can connect
#define BUFFER_SIZE 1024        // Max size for messages

int clients[MAX_CLIENTS];       // Array to keep track of connected clients
char nicknames[MAX_CLIENTS][50]; // Stores nicknames per client
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // Mutex to avoid race conditions

// Thread function to handle communication with a single client
void *handle_client(void *arg)
{
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int read_size;

    // Set default nicknames "User1", "User2", etc.
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client_socket) {
            snprintf(nicknames[i], sizeof(nicknames[i]), "User%d", i + 1);
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    // Keep reading messages from this client
    while ((read_size = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[read_size] = '\0'; // Null-terminate the message
    
        // Check for special commands
        if (strncmp(buffer, "/nick ", 6) == 0) {
            handle_nick(client_socket, buffer + 6, clients, nicknames);
            continue;
        }

        if (strncmp(buffer, "/list", 5) == 0) {
            handle_list(client_socket, clients, nicknames);
            continue;
        }

        // Check for `/msg group` command BEFORE `/msg`
        if (strncmp(buffer, "/msg group ", 11) == 0) {
            handle_group_msg(client_socket, buffer + 11, clients, nicknames);
            continue;
        }

        if (strncmp(buffer, "/msg ", 5) == 0) {
            handle_msg(client_socket, buffer + 5, clients, nicknames);
            continue;
        }

        if (strncmp(buffer, "/quit", 5) == 0) {
            int should_exit = handle_quit(client_socket, clients, nicknames);
            if (should_exit) break; // Exit the thread cleanly
        }

        if (strncmp(buffer, "/group ", 7) == 0) {
            handle_group(client_socket, buffer + 7, clients, nicknames);
            continue;
        }

        if (strncmp(buffer, "/react ", 7) == 0) {
            handle_react(client_socket, buffer + 7, clients, nicknames);
            continue;
        }

        if (strncmp(buffer, "/help", 5) == 0) { // Check for /help command
            handle_help(client_socket); // Call the help command handler
            continue;
        }

        // Loop through all clients and send this message to everyone except the sender
        pthread_mutex_lock(&lock); // Lock before accessing shared client list
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] && clients[i] != client_socket) {
                char full_msg[BUFFER_SIZE + 50];
                char sender_nick[50] = "Anonymous"; // Default nickname

                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (clients[j] == client_socket && strlen(nicknames[j]) > 0) {
                        strncpy(sender_nick, nicknames[j], sizeof(sender_nick));
                        break;
                    }
                }
                snprintf(full_msg, sizeof(full_msg), "%s: %s", sender_nick, buffer);
                send(clients[i], full_msg, strlen(full_msg), 0);
            }
        }
        pthread_mutex_unlock(&lock);
    }

    // If we get here, client disconnected or something went wrong
    close(client_socket);

    // Remove client from the list
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client_socket) {
            clients[i] = 0;
            memset(nicknames[i], 0, sizeof(nicknames[i]));
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL); // End the thread
    return NULL; // Null return for void *
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
