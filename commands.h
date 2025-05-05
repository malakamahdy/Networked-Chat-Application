#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>

#define MAX_CLIENTS 10         // Maximum number of clients
#define BUFFER_SIZE 1024       // Maximum size for messages

#define MAX_GROUPS 10          // Maximum number of groups
#define MAX_GROUP_MEMBERS 10   // Maximum members per group

typedef struct {
    char name[50];                // Group name
    int members[MAX_GROUP_MEMBERS]; // Sockets of group members
    int member_count;             // Number of members in the group
} Group;

// Declare global variables
extern Group groups[MAX_GROUPS];
extern int group_count;

// Function declarations
void handle_nick(int client_socket, char *new_nick, int clients[], char nicknames[][50]);
void handle_list(int client_socket, int clients[], char nicknames[][50]);
void handle_msg(int client_socket, char *input, int clients[], char nicknames[][50]);
int handle_quit(int client_socket, int clients[], char nicknames[][50]);
void handle_react(int client_socket, char *input, int clients[], char nicknames[][50]);
void handle_group(int client_socket, char *input, int clients[], char nicknames[][50]);
void handle_group_msg(int client_socket, char *input, int clients[], char nicknames[][50]);
void handle_help(int client_socket); 

#endif
