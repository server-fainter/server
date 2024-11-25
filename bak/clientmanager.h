#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <pthread.h>

#define MAX_CLIENTS 1024 // 최대 클라이언트 수

typedef struct {
    int client_sockets[MAX_CLIENTS]; // 클라이언트 소켓 배열
    int num_clients;                // 현재 연결된 클라이언트 수
    pthread_mutex_t lock;           // 동기화를 위한 뮤텍스
} ClientManager;

// 클라이언트 매니저 초기화 함수
void init_clients(ClientManager *cm);

// 새로운 클라이언트 추가 함수
void add_client(ClientManager *cm, int client_socket);

// 클라이언트 제거 함수
void remove_client(ClientManager *cm, int client_socket);

#endif // CLIENTMANAGER_H
