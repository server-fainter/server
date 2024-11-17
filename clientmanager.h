#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#define MAX_CLIENTS 1024
#define BUFFER_SIZE 1024
#define PORT 8080

typedef struct {
    int sockets[MAX_CLIENTS];
    int count;
} ClientManager;


void init_clients(ClientManager *manager);
void add_client(ClientManager *manager, int client_fd);
void remove_client(ClientManager *manager, int client_fd);

#endif // SERVER_H