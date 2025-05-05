# Makefile

CC = gcc
CFLAGS = -Wall -pthread -I/opt/homebrew/opt/openssl/include -L/opt/homebrew/opt/openssl/lib -lssl -lcrypto

# Source files
SERVER_OBJS = server.c commands.c encryption.c
CLIENT_OBJS = client.c encryption.c

# Targets to build
TARGETS = server client

all: $(TARGETS)

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o server

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o client

clean:
	rm -f $(TARGETS)




