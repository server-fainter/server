// client.c
#include "clientmanager.h"
#include <string.h>

void init_clients(ClientManager *manager) {
    memset(manager, 0, sizeof(ClientManager));
}

void add_client(ClientManager *manager, int client_fd) {
    if (manager->count < MAX_CLIENTS) {
        manager->sockets[manager->count++] = client_fd;
    }
}

void remove_client(ClientManager *manager, int client_fd) {
    for (int i = 0; i < manager->count; i++) {
        if (manager->sockets[i] == client_fd) {
            manager->sockets[i] = manager->sockets[--manager->count];
            break;
        }
    }
}
