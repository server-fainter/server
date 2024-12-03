#include "server.h"
#include "clientmanager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>

// 파일 디스크립터를 non-blocking 모드로 설정하는 함수
// 이 함수는 유틸리티 기능을 수행해서 별도의 공용 파일 (예: utils.c)로 분리할 필요가 있음 아님말고
int set_nonblocking(int fd) {
    // 현재 파일 디스크립터 플래그 가져오기
    int flags = fcntl(fd, F_GETFL, 0); 
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }

    // non-blocking 플래그 추가
    flags |= O_NONBLOCK;  
    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }

    return 0;
}

// 서버 초기화 함수
void init_server(ServerManager *sm)
{
    // 서버 소켓 생성
    sm->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (sm->server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE); // 실패 시 프로그램 종료
    }

    // 서버 소켓을 non-blocking 모드로 설정
    if (set_nonblocking(sm->server_socket) == -1) {
        perror("set_nonblocking failed");
        close(sm->server_socket);
        exit(EXIT_FAILURE);
    }

    // 서버 주소 정보 설정
    struct sockaddr_in server_addr;
    memset((char*)&server_addr, '\0', sizeof(server_addr)); 
    server_addr.sin_family = AF_INET;                     
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");    
    server_addr.sin_port = htons(PORT);                    

    // 소켓과 주소 바인딩
    if (bind(sm->server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        close(sm->server_socket);
        exit(EXIT_FAILURE);
    }

    // 리슨 모드로 설정
    if (listen(sm->server_socket, MAX_CLIENTS) == -1) {
        perror("listen failed");
        close(sm->server_socket);
        exit(EXIT_FAILURE);
    }

    // epoll 인스턴스 생성
    sm->epoll_fd = epoll_create1(0); 
    if (sm->epoll_fd == -1) {
        perror("epoll_create1 failed");
        close(sm->server_socket);
        exit(EXIT_FAILURE);
    }

    // 서버 소켓을 epoll 이벤트에 등록
    sm->ev.events = EPOLLIN | EPOLLET; // 읽기 가능, 엣지 트리거 모드
    sm->ev.data.fd = sm->server_socket;
    if (epoll_ctl(sm->epoll_fd, EPOLL_CTL_ADD, sm->server_socket, &sm->ev) == -1) {
        perror("epoll_ctl: server_socket");
        close(sm->server_socket);
        close(sm->epoll_fd);
        exit(EXIT_FAILURE);
    }

    printf("Success Init Server!\nServer is listening on port %d\n", PORT);
}

// 서버 종료 함수
void close_server(ServerManager *sm){
    close(sm->server_socket);
    close(sm->epoll_fd);
}

// 새로운 클라이언트를 ACCEPT하는 함수
int accept_new_client(ServerManager *sm)
{
    // 클라이언트 연결 수락
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket = accept(sm->server_socket, (struct sockaddr*)&client_addr, &client_addr_len);

    if (client_socket == -1) {
        perror("accept failed");
        return -1;
    }

    // 클라이언트를 non-blocking 모드로 설정
    if (set_nonblocking(client_socket) == -1) {
        close(client_socket);
        return -1;
    }

    // 클라이언트를 epoll 이벤트에 등록
    sm->ev.events = EPOLLIN | EPOLLET; 
    sm->ev.data.fd = client_socket;
    if (epoll_ctl(sm->epoll_fd, EPOLL_CTL_ADD, client_socket, &sm->ev) == -1) {
        perror("epoll_ctl add client failed");
        close(client_socket);
    }

    // 클라이언트 연결 성공 메시지 출력
    printf("Accepted new client: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)); 
    return 0;
}
