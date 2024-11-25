#include "thread_pool.h"
#include "context.h"
#include "canvas.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <cjson/cJSON.h>

// 워커 스레드: 작업 큐에서 작업을 가져와 처리
static void *worker_thread(void *arg) {
    Context *ctx = (Context *)arg;
    TaskQueue *queue = &ctx->queue;

    while (1) {
        Task task = pop_task(queue); 

        switch (task.type) {
            
        case TASK_NEW_CLIENT:
            accept_new_client(&ctx->sm);
            break;

        case TASK_TEST_RECV_MESSAGE:
            printf("Received data: %s\n", task.data);
            break;

        case TASK_PIXEL_UPDATE: 
            cJSON *json = cJSON_Parse(task.data);
            if (json) {
                cJSON *x = cJSON_GetObjectItem(json, "x");
                cJSON *y = cJSON_GetObjectItem(json, "y");
                cJSON *color = cJSON_GetObjectItem(json, "color");

                if (cJSON_IsNumber(x) && cJSON_IsNumber(y) && cJSON_IsString(color)) {
                    int px = x->valueint;
                    int py = y->valueint;
                    int col = (int)strtol(color->valuestring, NULL, 16);
                    update_pixel(px, py, col); // 캔버스 업데이트
                }
                cJSON_Delete(json);
            }
            break;

        case TASK_SEND_STATIC_FILE:
            /*
            1. 클라이언트가 새로 접속했을 경우
            2. 요청 받은 HTML, CSS, JS, 전체 캔버스 정보가 담긴 JSON 파일 SEND
            */
            break;

        case TASK_BROADCAST:
            /*

            1. 수정된 픽셀에 대한 데이터(캐시 버퍼)를 JSON 형식의 문자열로 변환(직렬화)
            2. 현재 접속중인 모든 클라이언트에게 브로드 캐스팅
            (아니면 픽셀이 수정될 때마다 브로드 캐스팅 하는 것은 어떤지 고려가 필요하다)

            */
            break;
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
        pthread_create(&pool->threads[i], NULL, worker_thread, ctx);
    }
}

// 스레드 풀 삭제
void destroy_thread_pool(ThreadPool *pool) {
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_cancel(pool->threads[i]);           
        pthread_join(pool->threads[i], NULL);       
    }
}
