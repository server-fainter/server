#include "init_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>

int client_fds[MAX_CLIENTS];  // 클라이언트 파일 기술자 배열
int num_clients = 0;          // 현재 클라이언트 수

// 클라이언트 파일 기술자 추가
void add_client(int client_fd) {
    if (num_clients < MAX_CLIENTS) {
        client_fds[num_clients++] = client_fd;
    }
}

// 클라이언트 파일 기술자 제거
void remove_client(int client_fd) {
    for (int i = 0; i < num_clients; i++) {
        if (client_fds[i] == client_fd) {
            client_fds[i] = client_fds[--num_clients];
            break;
        }
    }
}

// 새로운 클라이언트를 ACCPET 하는 함수
int accept_new_cleint(int server_fd, int epoll_fd, struct epoll_event *event)
{
    // 새로운 클라이언트 연결 수락
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_fd == -1) {
        perror("accept failed");
        return -1;
    }

    // 클라이언트를 non-blocking 모드로 설정
    if (set_nonblocking(client_fd) == -1) {
        close(client_fd);
        return -1;
    }

    // 새 클라이언트를 epoll 이벤트에 추가
    event->events = EPOLLIN | EPOLLET;
    event->data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, event) == -1) {
        perror("epoll_ctl: client_fd");
        close(client_fd);
        return -1;
    }

    printf("Accepted new client: %d\n", client_fd);
    return client_fd;       
}   

int main() {

    int server_fd = init_server();
    int epoll_fd;
    if(server_fd == -1)
    {
        perror("Failed Server Init");
        exit(EXIT_FAILURE);
    }
    
    printf("server_fd : %d\n", server_fd);

    // epoll 인스턴스 생성
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 서버 소켓을 epoll 이벤트에 추가
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl: server_fd");
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    // epoll_wait가 감지한 이벤트들을 저장하기 위한 배열
    struct epoll_event events[MAX_EVENTS];
    printf("Success Init Server!\nServer is listening on port %d\n", PORT);

    // epoll을 통해 이벤트 감지 및 처리
    while (1) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait failed");
            break;
        }

        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == server_fd) {

                // 새로운 클라이언트 연결 수락
                int client_fd = accept_new_cleint(server_fd, epoll_fd, &event);
                add_client(client_fd);
            } 
            else {
                // 기존 클라이언트로부터 데이터 수신
                int client_fd = events[i].data.fd;
                char buffer[BUFFER_SIZE];
                int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

                if (bytes_read == -1) {
                    if (errno != EAGAIN) {
                        perror("read failed");
                        close(client_fd);
                    }
                    continue;
                } 
                else if (bytes_read == 0) {
                    // 클라이언트가 연결을 종료함
                    printf("Client %d disconnected\n", client_fd);
                    close(client_fd);
                    continue;
                }

                buffer[bytes_read] = '\0';
                printf("Received from client %d: %s\n", client_fd, buffer);

                // 클라이언트에게 동일한 데이터 반환
                if (write(client_fd, buffer, bytes_read) == -1) {
                    perror("write failed");
                    close(client_fd);
                }
            }
        }
    }

    // 리소스 해제
    close(server_fd);
    close(epoll_fd);
    return 0;
}
