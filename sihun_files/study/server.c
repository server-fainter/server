#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <libwebsockets.h>
#include <pthread.h>

#define PORT 8081
#define CANVAS_WIDTH 100
#define CANVAS_HEIGHT 100
#define MAX_CLIENTS 100

typedef struct {
    int x, y;
    char color[7]; // HEX 색상 값 (예: "#FF5733")
} PixelUpdate;

// 캔버스 상태
char canvas[CANVAS_HEIGHT][CANVAS_WIDTH][7];

// 클라이언트 목록
struct lws *clients[MAX_CLIENTS] = { NULL };
int client_count = 0;

// 동기화를 위한 뮤텍스
pthread_mutex_t canvas_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// 캔버스 초기화
void initialize_canvas() {
    for (int i = 0; i < CANVAS_HEIGHT; i++) {
        for (int j = 0; j < CANVAS_WIDTH; j++) {
            strcpy(canvas[i][j], "#FFFFFF"); // 기본 색상: 흰색
        }
    }
}

// 캔버스 직렬화
void serialize_canvas(char **output) {
    size_t buffer_size = CANVAS_WIDTH * CANVAS_HEIGHT * 15; // 충분히 큰 버퍼 크기 설정
    *output = (char *)malloc(buffer_size);
    if (!*output) {
        perror("Failed to allocate memory for canvas serialization");
        return;
    }

    strcpy(*output, "{ \"canvas\": [");
    for (int i = 0; i < CANVAS_HEIGHT; i++) {
        strcat(*output, "[");
        for (int j = 0; j < CANVAS_WIDTH; j++) {
            strcat(*output, "\"");
            strcat(*output, canvas[i][j]);
            strcat(*output, "\"");
            if (j < CANVAS_WIDTH - 1) strcat(*output, ",");
        }
        strcat(*output, "]");
        if (i < CANVAS_HEIGHT - 1) strcat(*output, ",");
    }
    strcat(*output, "] }");
}

// 캔버스 업데이트
void update_canvas(PixelUpdate update) {
    if (update.x >= 0 && update.x < CANVAS_WIDTH && update.y >= 0 && update.y < CANVAS_HEIGHT) {
        pthread_mutex_lock(&canvas_mutex);
        snprintf(canvas[update.y][update.x], sizeof(canvas[update.y][update.x]), "%s", update.color);
        pthread_mutex_unlock(&canvas_mutex);
        printf("Canvas updated: (%d, %d) -> %s\n", update.x, update.y, canvas[update.y][update.x]);
    } else {
        printf("Invalid coordinates: (%d, %d)\n", update.x, update.y);
    }
}

// 모든 클라이언트에 메시지 브로드캐스트
void broadcast_message(const char *message) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i]) {
            unsigned char *buffer = (unsigned char *)malloc(LWS_PRE + strlen(message) + 1);
            if (buffer) {
                memset(buffer, 0, LWS_PRE + strlen(message) + 1);
                memcpy(buffer + LWS_PRE, message, strlen(message));
                lws_write(clients[i], buffer + LWS_PRE, strlen(message), LWS_WRITE_TEXT);
                free(buffer);
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// WebSocket 메시지 처리
void handle_message(const char *message) {
    PixelUpdate update;

    if (sscanf(message, "%d,%d,%6s", &update.x, &update.y, update.color) == 3) {
        printf("Parsed message: (%d, %d, %s)\n", update.x, update.y, update.color);
        update_canvas(update); // 캔버스 업데이트

        // 변경된 픽셀 정보 브로드캐스트
        char pixel_data[50];
        snprintf(pixel_data, sizeof(pixel_data), "{ \"x\": %d, \"y\": %d, \"color\": \"%s\" }", update.x, update.y, update.color);
        broadcast_message(pixel_data);
    } else {
        printf("Invalid message format: %s\n", message);
    }
}

// WebSocket 콜백
static int websocket_callback(struct lws *wsi, enum lws_callback_reasons reason,
                              void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_RECEIVE: {
            char received_message[256] = {0};

            strncpy(received_message, (const char *)in, len);
            received_message[len] = '\0';
            printf("Received raw message: %s\n", received_message);

            handle_message(received_message);
            break;
        }
        case LWS_CALLBACK_ESTABLISHED: {
            printf("Client connected\n");

            pthread_mutex_lock(&clients_mutex);
            if (client_count < MAX_CLIENTS) {
                clients[client_count++] = wsi;

                char *canvas_data = NULL;
                serialize_canvas(&canvas_data);
                if (canvas_data) {
                    unsigned char *buffer = (unsigned char *)malloc(LWS_PRE + strlen(canvas_data) + 1);
                    if (buffer) {
                        memset(buffer, 0, LWS_PRE + strlen(canvas_data) + 1);
                        memcpy(buffer + LWS_PRE, canvas_data, strlen(canvas_data));
                        lws_write(wsi, buffer + LWS_PRE, strlen(canvas_data), LWS_WRITE_TEXT);
                        free(buffer);
                    }
                    free(canvas_data);
                }
            } else {
                printf("Max client limit reached, rejecting new client\n");
                lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL, (unsigned char *)"Max clients reached", 17);
                lws_callback_on_writable(wsi);
            }
            pthread_mutex_unlock(&clients_mutex);
            break;
        }
        case LWS_CALLBACK_CLOSED: {
            printf("Client disconnected\n");

            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < client_count; i++) {
                if (clients[i] == wsi) {
                    clients[i] = clients[--client_count];
                    clients[client_count] = NULL; // 제거된 클라이언트를 NULL로 설정
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            break;
        }
        default:
            break;
    }
    return 0;
}

// WebSocket 서버 시작
void websocket_server_start(int port) {
    struct lws_context_creation_info info;
    struct lws_protocols protocols[] = {
        { "http-only", websocket_callback, 0, 0 },
        { NULL, NULL, 0, 0 } // 종료
    };

    memset(&info, 0, sizeof(info));
    info.port = port;
    info.protocols = protocols;

    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "Failed to create context\n");
        return;
    }

    printf("WebSocket server started on port %d\n", port);
    while (1) {
        lws_service(context, 1000);
    }

    lws_context_destroy(context);
}

// 메인 함수
int main() {
    initialize_canvas();
    websocket_server_start(PORT);
    return 0;
}
