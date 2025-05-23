#include "encryption.h"
#include "shared_key.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "commands.h"

// Include encryption headers to enable secure communication

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
        unsigned char decrypted[BUFFER_SIZE];
        // Attempt to decrypt the received message before processing commands
        // Decrypt the incoming message using AES-256-CBC
        int decrypted_len = aes_decrypt((unsigned char *)buffer, read_size, (unsigned char *)aes_key, (unsigned char *)aes_iv, decrypted);
        if (decrypted_len <= 0) continue;
        decrypted[decrypted_len] = '\0';

        // Check for special commands
        if (strncmp((char *)decrypted, "/nick ", 6) == 0) {
            handle_nick(client_socket, (char *)decrypted + 6, clients, nicknames);
            continue;
        }

        if (strncmp((char *)decrypted, "/list", 5) == 0) {
            handle_list(client_socket, clients, nicknames);
            continue;
        }

        // Check for `/msg group` command BEFORE `/msg`
        if (strncmp((char *)decrypted, "/msg group ", 11) == 0) {
            handle_group_msg(client_socket, (char *)decrypted + 11, clients, nicknames);
            continue;
        }

        if (strncmp((char *)decrypted, "/msg ", 5) == 0) {
            handle_msg(client_socket, (char *)decrypted + 5, clients, nicknames);
            continue;
        }

        if (strncmp((char *)decrypted, "/quit", 5) == 0) {
            int should_exit = handle_quit(client_socket, clients, nicknames);
            if (should_exit) break; // Exit the thread cleanly
        }

        if (strncmp((char *)decrypted, "/group ", 7) == 0) {
            handle_group(client_socket, (char *)decrypted + 7, clients, nicknames);
            continue;
        }

        if (strncmp((char *)decrypted, "/react ", 7) == 0) {
            handle_react(client_socket, (char *)decrypted + 7, clients, nicknames);
            continue;
        }

        if (strncmp((char *)decrypted, "/help", 5) == 0) { // Check for /help command
            handle_help(client_socket); // Call the help command handler
            continue;
        }

        // Broadcast the decrypted message to all connected clients except the sender
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
                snprintf(full_msg, sizeof(full_msg), "%s: %s", sender_nick, decrypted);
                // Encrypt the broadcast message before sending to each client
                unsigned char encrypted[BUFFER_SIZE];
                int encrypted_len = aes_encrypt((unsigned char *)full_msg, strlen(full_msg), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
                send(clients[i], encrypted, encrypted_len, 0);
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
