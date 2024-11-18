#include "clientmanager.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// 클라이언트 매니저 초기화
void init_clients(ClientManager *cm) {
    cm->num_clients = 0;
    memset(cm->client_sockets, -1, sizeof(cm->client_sockets)); // 초기화: -1로 설정
    pthread_mutex_init(&cm->lock, NULL);
}

// 새로운 클라이언트 추가
void add_client(ClientManager *cm, int client_socket) {
    pthread_mutex_lock(&cm->lock);

    if (cm->num_clients < MAX_CLIENTS) {
        cm->client_sockets[cm->num_clients++] = client_socket; // 소켓 추가
        printf("Client added: %d (Total: %d)\n", client_socket, cm->num_clients);
    } else {
        printf("Max clients reached. Rejecting client: %d\n", client_socket);
        close(client_socket); // 연결 해제
    }

    pthread_mutex_unlock(&cm->lock);
}

// 클라이언트 제거
void remove_client(ClientManager *cm, int client_socket) {
    pthread_mutex_lock(&cm->lock);

    for (int i = 0; i < cm->num_clients; i++) {
        if (cm->client_sockets[i] == client_socket) {
            // 배열에서 클라이언트를 제거하고 앞으로 이동
            for (int j = i; j < cm->num_clients - 1; j++) {
                cm->client_sockets[j] = cm->client_sockets[j + 1];
            }
            cm->client_sockets[cm->num_clients - 1] = -1; // 마지막 소켓 초기화
            cm->num_clients--;
            printf("Client removed: %d (Total: %d)\n", client_socket, cm->num_clients);
            break;
        }
    }

    pthread_mutex_unlock(&cm->lock);
}
