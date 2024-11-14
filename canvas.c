#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#define CANVAS_WIDTH 100
#define CANVAS_HEIGHT 100

extern int clients_num; 
extern int client_sockets[];

int canvas[CANVAS_WIDTH][CANVAS_HEIGHT];
pthread_mutex_t mutex;

void init_canvas(){ //캔버스 초기화
  for(int i = 0; i < CANVAS_WIDTH; i++){
    for(int j = 0; j < CANVAS_HEIGHT; j++){
      canvas[i][j] = 0;
    }
  }
}

int color_to_int(const char *hex){ 
  return (int)strtol(hex, NULL, 16); //16진수의 문자열을 int형으로 변환
}

void update_pixel(int x, int y, int color){ //캔버스에 픽셀 색상 업데이트
  if(x < 0 || x >= CANVAS_WIDTH || y < 0 || y >= CANVAS_HEIGHT){
    return;
  }
  pthread_mutex_lock(&mutex);//캔버스에 같은 값 동시에 접근방지
  canvas[x][y] = color;
  pthread_mutex_unlock(&mutex); //늦게 들어온 값이 적용되기 위함
}

void *canvas_broadcast(void *none){ //클라이언트에게 캔버스 전송
  int s_error;
  while(1){
    sleep(3);
    pthread_mutex_lock(&mutex);
    for(int i = 0; i < clients_num; i++){
      s_error = send(client_sockets[i], canvas, sizeof(canvas), 0); //우선 전체 캔버스를 전송, 후에 변경된 부분만 전송하도록 수정
      if(s_error){
        perror("send");
      }
    }
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

int main(void)
{
  init_canvas();
  return 0;
}