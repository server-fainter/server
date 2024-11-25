#include "context.h"
#include "task_queue.h"
#include "thread_pool.h"
#include "server.h"
#include "canvas.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>


int main() {
    // Context 구조체 초기화
    Context ctx;

    init_server(&ctx.sm);             // 서버 초기화
    init_clients(&ctx.cm);            // 클라이언트 관리 초기화
    init_task_queue(&ctx.queue);      // 작업 큐 초기화
    init_canvas();                    // 캔버스 초기화

    // 스레드 풀 및 브로드캐스트 스레드 초기화
    ThreadPool pool;
    init_thread_pool(&pool, &ctx);

    pthread_t broadcast_thread;
    pthread_create(&broadcast_thread, NULL, canvas_broadcast, &ctx);

    // 이벤트 루프
    while (1) {
        int num_events = epoll_wait(ctx.sm.epoll_fd, ctx.sm.events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait failed");
            continue;
        }

        for (int i = 0; i < num_events; i++) 
        {
            if (ctx.sm.events[i].data.fd == ctx.sm.server_socket) 
            {
                // 서버 소켓에서 새로운 클라이언트 연결 처리
                Task task = {ctx.sm.server_socket, TASK_NEW_CLIENT, ""};
                push_task(&ctx.queue, task);

            } 
            else 
            {
                // 클라이언트 소켓에서 데이터 수신
                char buffer[BUFFER_SIZE];
                int client_fd = ctx.sm.events[i].data.fd;
                int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0'; // Null-terminate string
                    Task task = {client_fd, TASK_PIXEL_UPDATE, ""};
                    strncpy(task.data, buffer, sizeof(task.data) - 1);
                    push_task(&ctx.queue, task);
                } 
                else {
                    // 클라이언트 연결 종료 처리
                    close(client_fd);
                    printf("Client disconnected: %d\n", client_fd);
                }
            }
        }
    }

    // 리소스 정리
    close_server(&ctx.sm);
    destroy_thread_pool(&pool);
    pthread_mutex_destroy(&mutex);

    return 0;
}
