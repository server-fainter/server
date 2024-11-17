#ifndef SERVER_H
#define SERVER_H

#include <sys/epoll.h>

#define PORT 8080            // 서버 포트
#define MAX_EVENTS 1024       // epoll 최대 이벤트 수
#define BUFFER_SIZE 1024     // 클라이언트와의 메시지 버퍼 크기

typedef struct{
    int server_socket;
    int epoll_fd;
    struct epoll_event ev;  // 디스크립터에 대해 감시할 이벤트를 정의할 때 사용
    struct epoll_event events[MAX_EVENTS]; // epoll에서 감지된 여러 이벤트를 저장하는 배열
}ServerManager;

// 파일 디스크립터를 non-blocking 모드로 설정하는 함수
int set_nonblocking(int fd);

// 서버 초기화 함수
void init_server(ServerManager *sm);

int accept_new_client(ServerManager *sm);

void close_server(ServerManager *sm);

#endif // SERVER_H
