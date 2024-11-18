#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <pthread.h>

// 작업 큐 크기 정의
#define QUEUE_SIZE 1024

// 작업(Task) 구조체
typedef struct {
    int fd;   // 작업과 관련된 파일 디스크립터
    int type; // 작업 유형 (0: Accept, 1: Read, 2: Canvas Update)
    char data[256]; // 클라이언트로부터 받은 데이터 (예: JSON)
} Task;

// 작업 큐(Task Queue) 구조체
typedef struct {
    Task tasks[QUEUE_SIZE]; // 작업 배열 (고정 크기 큐)
    int front, rear;        // 큐의 앞(front)과 뒤(rear) 인덱스
    pthread_mutex_t lock;   // 뮤텍스
    pthread_cond_t cond;    // 조건 변수
} TaskQueue;

// 작업 큐 초기화 함수
void init_task_queue(TaskQueue *queue);

// 작업을 큐에 추가하는 함수
void push_task(TaskQueue *queue, Task task);

// 작업을 큐에서 가져오는 함수
Task pop_task(TaskQueue *queue);

#endif // TASK_QUEUE_H
