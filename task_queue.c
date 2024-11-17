#include "task_queue.h"
#include <stdlib.h>
#include <stdio.h>

// 작업 큐 초기화
void init_task_queue(TaskQueue *queue) {
    queue->front = queue->rear = 0;
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->cond, NULL);
}

// Task Queue Push
void push_task(TaskQueue *queue, Task task) {
    pthread_mutex_lock(&queue->lock);

    // Insert the task into the queue
    queue->tasks[queue->rear] = task;
    queue->rear = (queue->rear + 1) % QUEUE_SIZE;

    // Signal waiting threads
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->lock);
}

// Task Queue Pop
Task pop_task(TaskQueue *queue) {
    pthread_mutex_lock(&queue->lock);

    // Wait for a task to become available
    while (queue->front == queue->rear) {
        pthread_cond_wait(&queue->cond, &queue->lock);
    }

    // Get the task
    Task task = queue->tasks[queue->front];
    queue->front = (queue->front + 1) % QUEUE_SIZE;

    pthread_mutex_unlock(&queue->lock);
    return task;
}
