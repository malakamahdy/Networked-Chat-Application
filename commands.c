// commands.c

#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "commands.h"
#include <unistd.h>  
#include "encryption.h"
#include "shared_key.h"

extern pthread_mutex_t lock;

// Define groups and count
Group groups[MAX_GROUPS] = {0};
int group_count = 0;

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
    // Encrypt the outgoing server response before sending to the client
    unsigned char encrypted[BUFFER_SIZE];
    int encrypted_len = aes_encrypt((unsigned char *)confirm_msg, strlen(confirm_msg), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
    send(client_socket, encrypted, encrypted_len, 0);
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
    // Encrypt the outgoing server response before sending to the client
    unsigned char encrypted[BUFFER_SIZE];
    int encrypted_len = aes_encrypt((unsigned char *)list_msg, strlen(list_msg), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
    send(client_socket, encrypted, encrypted_len, 0);
}

// This function handles the /msg command

void handle_msg(int client_socket, char *input, int clients[], char nicknames[][50]) {
    char *target_nick = strtok(input, " ");       // Get the nickname
    char *message_body = strtok(NULL, "");        // Get the rest of the message

    if (!target_nick || !message_body) {
        char *err = "Incorrect Usage: /msg <nickname> <message>\n";
        // Encrypt the outgoing server response before sending to the client
        unsigned char encrypted[BUFFER_SIZE];
        int encrypted_len = aes_encrypt((unsigned char *)err, strlen(err), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
        send(client_socket, encrypted, encrypted_len, 0);
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
            // Encrypt the outgoing server response before sending to the client
            unsigned char encrypted[BUFFER_SIZE];
            int encrypted_len = aes_encrypt((unsigned char *)private_msg, strlen(private_msg), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
            send(clients[i], encrypted, encrypted_len, 0);
            target_found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&lock);

    if (!target_found) {
        char err_msg[BUFFER_SIZE];
        snprintf(err_msg, sizeof(err_msg), "No user found with nickname '%s'\n", target_nick);
        // Encrypt the outgoing server response before sending to the client
        unsigned char encrypted[BUFFER_SIZE];
        int encrypted_len = aes_encrypt((unsigned char *)err_msg, strlen(err_msg), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
        send(client_socket, encrypted, encrypted_len, 0);
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
            // Encrypt the outgoing server response before sending to the client
            unsigned char encrypted[BUFFER_SIZE];
            int encrypted_len = aes_encrypt((unsigned char *)goodbye_msg, strlen(goodbye_msg), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
            send(clients[i], encrypted, encrypted_len, 0);
        }
    }
    pthread_mutex_unlock(&lock);

    // Tell the quitting client goodbye and close their socket
    // Encrypt the outgoing server response before sending to the client
    unsigned char encrypted[BUFFER_SIZE];
    int encrypted_len = aes_encrypt((unsigned char *)"Disconnected from server.\n", 27, (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
    send(client_socket, encrypted, encrypted_len, 0);
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
        // Encrypt the outgoing server response before sending to the client
        unsigned char encrypted[BUFFER_SIZE];
        int encrypted_len = aes_encrypt((unsigned char *)err, strlen(err), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
        send(client_socket, encrypted, encrypted_len, 0);
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
        // Encrypt the outgoing server response before sending to the client
        unsigned char encrypted[BUFFER_SIZE];
        int encrypted_len = aes_encrypt((unsigned char *)err, strlen(err), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
        send(client_socket, encrypted, encrypted_len, 0);
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
            // Encrypt the outgoing server response before sending to the client
            unsigned char encrypted[BUFFER_SIZE];
            int encrypted_len = aes_encrypt((unsigned char *)reaction_msg, strlen(reaction_msg), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
            send(clients[i], encrypted, encrypted_len, 0);
            target_found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&lock);

    if (!target_found) {
        char err_msg[BUFFER_SIZE];
        snprintf(err_msg, sizeof(err_msg), "No user found with nickname '%s'\n", target_nick);
        // Encrypt the outgoing server response before sending to the client
        unsigned char encrypted[BUFFER_SIZE];
        int encrypted_len = aes_encrypt((unsigned char *)err_msg, strlen(err_msg), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
        send(client_socket, encrypted, encrypted_len, 0);
    }
}

void handle_group(int client_socket, char *input, int clients[], char nicknames[][50]) {
    // Tokenize input to extract group name, members, and message
    char *member_list = strtok(input, " ");   // Members
    char *group_name = strtok(NULL, " ");    // Group name
    char *message = strtok(NULL, "");        // Optional message

    if (!member_list || !group_name) {
        char *err = "Incorrect Usage: /group <member1>,<member2>,... <group_name> [message]\n";
        // Encrypt the outgoing server response before sending to the client
        unsigned char encrypted[BUFFER_SIZE];
        int encrypted_len = aes_encrypt((unsigned char *)err, strlen(err), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
        send(client_socket, encrypted, encrypted_len, 0);
        return;
    }

    // Check if group name is unique
    pthread_mutex_lock(&lock);
    for (int i = 0; i < group_count; i++) {
        if (strcmp(groups[i].name, group_name) == 0) {
            char *err = "Group name already exists.\n";
            // Encrypt the outgoing server response before sending to the client
            unsigned char encrypted[BUFFER_SIZE];
            int encrypted_len = aes_encrypt((unsigned char *)err, strlen(err), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
            send(client_socket, encrypted, encrypted_len, 0);
            pthread_mutex_unlock(&lock);
            return;
        }
    }

    // Create new group
    if (group_count >= MAX_GROUPS) {
        char *err = "Maximum number of groups reached.\n";
        // Encrypt the outgoing server response before sending to the client
        unsigned char encrypted[BUFFER_SIZE];
        int encrypted_len = aes_encrypt((unsigned char *)err, strlen(err), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
        send(client_socket, encrypted, encrypted_len, 0);
        pthread_mutex_unlock(&lock);
        return;
    }

    Group *new_group = &groups[group_count++];
    strncpy(new_group->name, group_name, sizeof(new_group->name));
    new_group->member_count = 0;

    // Add the sender to the group
    new_group->members[new_group->member_count++] = client_socket;

    // Add other members to the group
    char *member_nick = strtok(member_list, ",");
    while (member_nick != NULL) {
        int member_found = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != 0 && strcmp(nicknames[i], member_nick) == 0) {
                new_group->members[new_group->member_count++] = clients[i];
                member_found = 1;
                break;
            }
        }
        if (!member_found) {
            char err_msg[BUFFER_SIZE];
            snprintf(err_msg, sizeof(err_msg), "No user found with nickname '%s'\n", member_nick);
            // Encrypt the outgoing server response before sending to the client
            unsigned char encrypted[BUFFER_SIZE];
            int encrypted_len = aes_encrypt((unsigned char *)err_msg, strlen(err_msg), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
            send(client_socket, encrypted, encrypted_len, 0);
        }
        member_nick = strtok(NULL, ",");
    }
    pthread_mutex_unlock(&lock);

    // Notify group members
    char group_msg[BUFFER_SIZE + 100];
    snprintf(group_msg, sizeof(group_msg), "Group '%s' created.\n", group_name);
    for (int i = 0; i < new_group->member_count; i++) {
        // Encrypt the outgoing server response before sending to the client
        unsigned char encrypted[BUFFER_SIZE];
        int encrypted_len = aes_encrypt((unsigned char *)group_msg, strlen(group_msg), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
        send(new_group->members[i], encrypted, encrypted_len, 0);
    }

    // Send initial message if provided
    if (message) {
        snprintf(group_msg, sizeof(group_msg), "[Group %s] %s\n", group_name, message);
        for (int i = 0; i < new_group->member_count; i++) {
            // Encrypt the outgoing server response before sending to the client
            unsigned char encrypted[BUFFER_SIZE];
            int encrypted_len = aes_encrypt((unsigned char *)group_msg, strlen(group_msg), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
            send(new_group->members[i], encrypted, encrypted_len, 0);
        }
    }
}

void handle_group_msg(int client_socket, char *input, int clients[], char nicknames[][50]) {
    // Tokenize input to extract group name and message
    char *group_name = strtok(input, " ");
    char *message = strtok(NULL, "");

    if (!group_name || !message) {
        char *err = "Incorrect Usage: /msg group <group_name> <message>\n";
        // Encrypt the outgoing server response before sending to the client
        unsigned char encrypted[BUFFER_SIZE];
        int encrypted_len = aes_encrypt((unsigned char *)err, strlen(err), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
        send(client_socket, encrypted, encrypted_len, 0);
        return;
    }

    pthread_mutex_lock(&lock);

    // Find the group
    Group *target_group = NULL;
    for (int i = 0; i < group_count; i++) {
        if (strcmp(groups[i].name, group_name) == 0) {
            target_group = &groups[i];
            break;
        }
    }

    if (!target_group) {
        char *err = "Group not found.\n";
        // Encrypt the outgoing server response before sending to the client
        unsigned char encrypted[BUFFER_SIZE];
        int encrypted_len = aes_encrypt((unsigned char *)err, strlen(err), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
        send(client_socket, encrypted, encrypted_len, 0);
        pthread_mutex_unlock(&lock);
        return;
    }

    // Check if sender is a member
    int is_member = 0;
    for (int i = 0; i < target_group->member_count; i++) {
        if (target_group->members[i] == client_socket) {
            is_member = 1;
            break;
        }
    }

    if (!is_member) {
        char *err = "You are not a member of this group.\n";
        // Encrypt the outgoing server response before sending to the client
        unsigned char encrypted[BUFFER_SIZE];
        int encrypted_len = aes_encrypt((unsigned char *)err, strlen(err), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
        send(client_socket, encrypted, encrypted_len, 0);
        pthread_mutex_unlock(&lock);
        return;
    }

    // Send message to all group members
    char group_msg[BUFFER_SIZE + 100];
    char sender_nick[50] = "Anonymous";

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client_socket && strlen(nicknames[i]) > 0) {
            strncpy(sender_nick, nicknames[i], sizeof(sender_nick));
            break;
        }
    }

    snprintf(group_msg, sizeof(group_msg), "[Group %s] %s: %s\n", group_name, sender_nick, message);
    for (int i = 0; i < target_group->member_count; i++) {
        // Encrypt the outgoing server response before sending to the client
        unsigned char encrypted[BUFFER_SIZE];
        int encrypted_len = aes_encrypt((unsigned char *)group_msg, strlen(group_msg), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
        send(target_group->members[i], encrypted, encrypted_len, 0);
    }

    pthread_mutex_unlock(&lock);
}

// Function to handle the /help command
void handle_help(int client_socket)
{
    const char *help_message =
        "Here are the list of commands available to users:\n\n"
        "1. Change Nickname:\n"
        "   /nick <new_nickname>\n"
        "   Example: /nick john\n\n"
        "2. List Online Users:\n"
        "   /list\n"
        "   Example: /list\n\n"
        "3. Send a Private Message:\n"
        "   /msg <nickname> <message>\n"
        "   Example: /msg alice Hello, Alice!\n\n"
        "4. React to a Message:\n"
        "   /react <nickname> <reaction>\n"
        "   Valid reactions: laugh, love, emphasize, question\n"
        "   Example: /react john laugh\n\n"
        "5. Create a Group Chat:\n"
        "   /group <nickname1>,<nickname2>,... <group_name> [message]\n"
        "   Example: /group alice,bob teamchat Hello team!\n\n"
        "6. Send a Message to a Group:\n"
        "   /msg group <group_name> <message>\n"
        "   Example: /msg group teamchat Let's plan the project!\n\n"
        "7. Quit the Application:\n"
        "   /quit\n"
        "   Example: /quit\n\n"
        "8. See available commands:\n"
        "   /help\n"
        "   Example: /help\n\n";

    // Encrypt the outgoing server response before sending to the client
    unsigned char encrypted[BUFFER_SIZE * 2];
    int encrypted_len = aes_encrypt((unsigned char *)help_message, strlen(help_message), (unsigned char *)aes_key, (unsigned char *)aes_iv, encrypted);
    send(client_socket, encrypted, encrypted_len, 0);
}
