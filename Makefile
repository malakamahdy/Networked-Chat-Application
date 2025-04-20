# Makefile

CC = gcc
CFLAGS = -Wall -pthread

# Source files
SERVER_OBJS = server.c commands.c
CLIENT_OBJS = client.c

# Targets to build
TARGETS = server client

all: $(TARGETS)

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o server

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o client

clean:
	rm -f $(TARGETS)




