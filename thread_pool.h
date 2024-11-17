#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "task_queue.h"
#include "context.h"

#define THREAD_POOL_SIZE 4

typedef struct {
    pthread_t threads[THREAD_POOL_SIZE];
} ThreadPool;

// 스레드 풀 초기화
void init_thread_pool(ThreadPool *pool, Context *queue);

// 스레드 풀 삭제
void destroy_thread_pool(ThreadPool *pool);

#endif // THREAD_POOL_H
