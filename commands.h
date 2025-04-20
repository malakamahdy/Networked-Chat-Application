// commands.h

#ifndef COMMANDS_H
#define COMMANDS_H

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

void handle_nick(int client_socket, char *new_nick, int clients[], char nicknames[][50]);
void handle_list(int client_socket, int clients[], char nicknames[][50]);
void handle_msg(int client_socket, char *input, int clients[], char nicknames[][50]);
int handle_quit(int client_socket, int clients[], char nicknames[][50]);

#endif
