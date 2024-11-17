#include "thread_pool.h"
#include "context.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

static void *worker_thread(void *arg) {
    Context *ctx = (Context *)arg; // Context 참조
    TaskQueue *queue = &ctx->queue;

    while (1) {
        Task task = pop_task(queue);

        switch (task.type) 
        {
        case 0: { 
            // 새로운 연결 처리
            accept_new_client(&ctx->sm);
            add_client(&ctx->cm, task.fd);
            break;
        }
        case 1: {
             // 클라이언트 요청 처리
            char buffer[1024];
            ssize_t bytes_read = recv(task.fd, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0) {
                perror("recv failed");
            } else {
                buffer[bytes_read] = '\0';
                printf("Received from client %d: %s\n", task.fd, buffer);
                send(task.fd, "nice", bytes_read, 0);
            }
            break;
        }
        default:
            fprintf(stderr, "Unknown task type: %d\n", task.type);
            break;
        }
    }
    return NULL;
}

// 스레드 풀 초기화
void init_thread_pool(ThreadPool *pool, Context *ctx) {
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&pool->threads[i], NULL, worker_thread, ctx); // Context 전달
    }
}


// 스레드 풀 삭제
void destroy_thread_pool(ThreadPool *pool) {
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_cancel(pool->threads[i]);
        pthread_join(pool->threads[i], NULL);
    }
}
