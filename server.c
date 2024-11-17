#include "init_server.h"
#include "clientmanager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>


// 새로운 클라이언트를 ACCPET 하는 함수
int accept_new_client(int server_socket, int epoll_fd, struct epoll_event *event)
{
    // 새로운 클라이언트 연결 수락
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_socket == -1) {
        perror("accept failed");
        return -1;
    }

    // 클라이언트를 non-blocking 모드로 설정
    if (set_nonblocking(client_socket) == -1) {
        close(client_socket);
        return -1;
    }

    // 새 클라이언트를 epoll 이벤트에 추가
    event->events = EPOLLIN | EPOLLET;
    event->data.fd = client_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, event) == -1) {
        perror("epoll_ctl: client_fd");
        close(client_socket);
        return -1;
    }

    printf("Accepted new client: %d\n", client_socket);
    return client_socket;       
}   


int main() {
    
    ClientManager clientmanager;
    init_clients(&clientmanager);
    
    int server_socket = init_server();
    if(server_socket == -1)
    {
        perror("Failed Server Init");
        exit(EXIT_FAILURE);
    }
    
    //printf("server_socket : %d\n", server_socket);

    int epoll_fd;
    // epoll 인스턴스 생성
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // 서버 소켓을 epoll 이벤트에 추가
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) {
        perror("epoll_ctl: server_socket");
        close(server_socket);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    // epoll_wait가 감지한 이벤트들을 저장하기 위한 배열
    struct epoll_event events[MAX_EVENTS];

    // epoll을 통해 이벤트 감지 및 처리
    while (1) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait failed");
            continue;
        }

        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == server_socket) 
            {

                // 새로운 클라이언트 연결 수락
                int client_socket = accept_new_client(server_socket, epoll_fd, &event);
                if(client_socket > 0) {
                    add_client(&clientmanager, client_socket);
                }
                else {
                    perror("Client Accpet Error");
                }
                
            }
            else 
            {
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
    close(server_socket);
    close(epoll_fd);
    return 0;
}
