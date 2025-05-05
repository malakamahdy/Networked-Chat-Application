// commands.c

#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "commands.h"
#include <unistd.h>  

extern pthread_mutex_t lock;

// This function handles the /nick command

void handle_nick(int client_socket, char *new_nick, int clients[], char nicknames[][50]) {
    new_nick[strcspn(new_nick, "\n")] = 0; // Remove newline

    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client_socket) {
            strncpy(nicknames[i], new_nick, sizeof(nicknames[i]) - 1);
            nicknames[i][sizeof(nicknames[i]) - 1] = '\0';
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    char confirm_msg[BUFFER_SIZE];
    snprintf(confirm_msg, sizeof(confirm_msg), "Nickname changed to *%s*\n", new_nick);
    send(client_socket, confirm_msg, strlen(confirm_msg), 0);
}

// This function handles the /list command

void handle_list(int client_socket, int clients[], char nicknames[][50]) {
    char list_msg[BUFFER_SIZE] = "Connected users:\n";

    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != 0 && strlen(nicknames[i]) > 0) {
            strcat(list_msg, "- ");
            strcat(list_msg, nicknames[i]);
            strcat(list_msg, "\n");
        }
    }
    pthread_mutex_unlock(&lock);

    send(client_socket, list_msg, strlen(list_msg), 0);
}

// This function handles the /msg command

void handle_msg(int client_socket, char *input, int clients[], char nicknames[][50]) {
    char *target_nick = strtok(input, " ");       // Get the nickname
    char *message_body = strtok(NULL, "");        // Get the rest of the message

    if (!target_nick || !message_body) {
        char *err = "Incorrect Usage: /msg <nickname> <message>\n";
        send(client_socket, err, strlen(err), 0);
        return;
    }

    int target_found = 0;
    char sender_nick[50] = "Anonymous";

    pthread_mutex_lock(&lock);

    // Get sender's nickname
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client_socket && strlen(nicknames[i]) > 0) {
            strncpy(sender_nick, nicknames[i], sizeof(sender_nick));
            break;
        }
    }

    // Send to the intended recipient
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != 0 && strcmp(nicknames[i], target_nick) == 0) {
            char private_msg[BUFFER_SIZE + 100];
            snprintf(private_msg, sizeof(private_msg), "[Private Msg] %s: %s\n", sender_nick, message_body);
            send(clients[i], private_msg, strlen(private_msg), 0);
            target_found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&lock);

    if (!target_found) {
        char err_msg[BUFFER_SIZE];
        snprintf(err_msg, sizeof(err_msg), "No user found with nickname '%s'\n", target_nick);
        send(client_socket, err_msg, strlen(err_msg), 0);
    }
}

// This function handles the /quit command

int handle_quit(int client_socket, int clients[], char nicknames[][50]) {
    char user_nick[50] = "Anonymous";

    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client_socket) {
            if (strlen(nicknames[i]) > 0) {
                strncpy(user_nick, nicknames[i], sizeof(user_nick));
            }

            clients[i] = 0;
            memset(nicknames[i], 0, sizeof(nicknames[i]));
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    char goodbye_msg[BUFFER_SIZE];
    snprintf(goodbye_msg, sizeof(goodbye_msg), "[QUIT]: %s has left the chat.\n", user_nick);

    // Broadcast to all remaining users
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != 0 && clients[i] != client_socket) {
            send(clients[i], goodbye_msg, strlen(goodbye_msg), 0);
        }
    }
    pthread_mutex_unlock(&lock);

    // Tell the quitting client goodbye and close their socket
    send(client_socket, "Disconnected from server.\n", 27, 0);
    close(client_socket);
    return 1; // Signal that the client should exit the thread
}

void handle_react(int client_socket, char *input, int clients[], char nicknames[][50]) {
    // Tokenize the input to extract the target nickname and reaction
    char *target_nick = strtok(input, " ");   // Get the nickname of the target user
    char *reaction = strtok(NULL, "");       // Get the reaction type

    // Validate the input
    if (!target_nick || !reaction) {
        char *err = "Incorrect Usage: /react <nickname> <reaction>\n";
        send(client_socket, err, strlen(err), 0);
        return;
    }

    // Trim any trailing newlines from the reaction
    reaction[strcspn(reaction, "\n")] = '\0';

    // Validate the reaction type
    const char *valid_reactions[] = {"laugh", "love", "emphasize", "question"};
    int is_valid_reaction = 0;
    for (int i = 0; i < 4; i++) {
        if (strcmp(reaction, valid_reactions[i]) == 0) {
            is_valid_reaction = 1;
            break;
        }
    }

    if (!is_valid_reaction) {
        char *err = "Invalid reaction. Valid reactions are: laugh, love, emphasize, question.\n";
        send(client_socket, err, strlen(err), 0);
        return;
    }

    int target_found = 0;
    char sender_nick[50] = "Anonymous";

    pthread_mutex_lock(&lock);

    // Get the sender's nickname
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client_socket && strlen(nicknames[i]) > 0) {
            strncpy(sender_nick, nicknames[i], sizeof(sender_nick));
            break;
        }
    }

    // Find the target user and send the reaction
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != 0 && strcmp(nicknames[i], target_nick) == 0) {
            char reaction_msg[BUFFER_SIZE + 100];
            snprintf(reaction_msg, sizeof(reaction_msg), "%s reacted with '%s' to your message.\n", sender_nick, reaction);
            send(clients[i], reaction_msg, strlen(reaction_msg), 0);
            target_found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&lock);

    if (!target_found) {
        char err_msg[BUFFER_SIZE];
        snprintf(err_msg, sizeof(err_msg), "No user found with nickname '%s'\n", target_nick);
        send(client_socket, err_msg, strlen(err_msg), 0);
    }
}
