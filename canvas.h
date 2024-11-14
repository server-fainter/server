#ifndef CANVAS_H
#define CANVAS_H

#include <pthread.h>

#define CANVAS_WIDTH 100
#define CANVAS_HEIGHT 100

extern int canvas[CANVAS_WIDTH][CANVAS_HEIGHT];
extern pthread_mutex_t mutex;

void init_canvas();
void update_pixel(int x, int y, int color);
void *canvas_broadcast(void *none);

#endif