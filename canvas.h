#ifndef CANVAS_H
#define CANVAS_H

#include <pthread.h>

#define CANVAS_WIDTH 100
#define CANVAS_HEIGHT 100

// 캔버스 배열
extern int canvas[CANVAS_WIDTH][CANVAS_HEIGHT];
extern pthread_mutex_t mutex;
// 캔버스 초기화 함수
void init_canvas();

// 캔버스의 특정 픽셀 업데이트
void update_pixel(int x, int y, int color);

// 캔버스 상태를 클라이언트로 브로드캐스트
void *canvas_broadcast(void *arg);

#endif // CANVAS_H
