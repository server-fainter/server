#include "canvas.h"
#include "context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cjson/cJSON.h>

int canvas[CANVAS_WIDTH][CANVAS_HEIGHT];
pthread_mutex_t mutex;

void init_canvas() { // 캔버스 초기화
    for (int i = 0; i < CANVAS_WIDTH; i++) {
        for (int j = 0; j < CANVAS_HEIGHT; j++) {
            canvas[i][j] = 0;
        }
    }
    pthread_mutex_init(&mutex, NULL);
}

void update_pixel(int x, int y, int color) { // 픽셀 업데이트
    if (x < 0 || x >= CANVAS_WIDTH || y < 0 || y >= CANVAS_HEIGHT) {
        return; // 범위 초과 시 무시
    }
    pthread_mutex_lock(&mutex); 
    canvas[x][y] = color; // 픽셀 색상 업데이트
    pthread_mutex_unlock(&mutex);
}

void *canvas_broadcast(void *arg) { // 캔버스 상태 브로드캐스트
    Context *ctx = (Context *)arg;

    while (1) {
        sleep(3); // 3초 간격으로 브로드캐스트

        pthread_mutex_lock(&mutex);

        // 캔버스를 JSON으로 변환
        cJSON *json_canvas = cJSON_CreateArray();
        for (int i = 0; i < CANVAS_WIDTH; i++) {
            for (int j = 0; j < CANVAS_HEIGHT; j++) {
                if (canvas[i][j] != 0) { // 비어 있지 않은 픽셀만 추가
                    cJSON *pixel = cJSON_CreateObject();
                    cJSON_AddNumberToObject(pixel, "x", i);
                    cJSON_AddNumberToObject(pixel, "y", j);
                    cJSON_AddNumberToObject(pixel, "color", canvas[i][j]);
                    cJSON_AddItemToArray(json_canvas, pixel);
                }
            }
        }

        char *json_string = cJSON_PrintUnformatted(json_canvas);

        // 연결된 모든 클라이언트에 JSON 전송
        for (int i = 0; i < ctx->cm.num_clients; i++) {
            int client_socket = ctx->cm.client_sockets[i];
            if (send(client_socket, json_string, strlen(json_string), 0) < 0) {
                perror("send failed");
            }
        }

        cJSON_Delete(json_canvas);
        free(json_string);

        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}
