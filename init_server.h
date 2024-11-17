#ifndef SERVER_H
#define SERVER_H

#define PORT 8080            // 서버 포트
#define MAX_EVENTS 1024       // epoll 최대 이벤트 수
#define BUFFER_SIZE 1024     // 클라이언트와의 메시지 버퍼 크기

// 파일 디스크립터를 non-blocking 모드로 설정하는 함수
int set_nonblocking(int fd);

// 서버 초기화 함수
int init_server();

#endif // SERVER_H
