#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void *receive_handler(void *socket_desc) {
    int sock = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];
    int read_size;
    while ((read_size = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        buffer[read_size] = '\0';
        printf("%s", buffer);
    }
    return 0;
}

int main() {
    int sock;
    struct sockaddr_in server;
    char message[BUFFER_SIZE];
    pthread_t recv_thread;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    connect(sock, (struct sockaddr *)&server, sizeof(server));
    pthread_create(&recv_thread, NULL, receive_handler, &sock);

    while (fgets(message, sizeof(message), stdin) != NULL) {
        send(sock, message, strlen(message), 0);
    }

    close(sock);
    return 0;
}
