#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <pthread.h>

#define QUEUE_SIZE 100

typedef struct {
    int fd;   // 파일 기술자
    int type; // 0: Accept, 1: Read
} Task;

typedef struct {
    Task tasks[QUEUE_SIZE];
    int front, rear;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} TaskQueue;

// 작업 큐 초기화
void init_task_queue(TaskQueue *queue);

// Task Queue Push!
void push_task(TaskQueue *queue, Task task);

// Task Queue Pop!
Task pop_task(TaskQueue *queue);

#endif // TASK_QUEUE_H