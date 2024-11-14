#include "init_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>

// 파일 디스크립터를 non-blocking 모드로 설정하는 함수
int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0); // 현재 플래그 상태 가져오기
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }

    flags |= O_NONBLOCK;  // non_blocking 모드로 성정
    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }

    return 0;
}

int init_server()
{
    int server_fd;
    struct sockaddr_in server_addr;

    // 서버 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        return -1;
    }

    // 서버 소켓을 non-blocking 모드로 설정
    if (set_nonblocking(server_fd) == -1) {
        perror("set_nonblocking failed");
        close(server_fd);
        return -1;
    }

    // 서버 주소 정보 설정
    memset((char*)&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    server_addr.sin_port = htons(PORT);

    // 바인딩
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        close(server_fd);
        return -1;
    }

    // 서버를 리슨 모드로 설정
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen failed");
        close(server_fd);
        return -1;
    }

    return server_fd;
}