#include <libwebsockets.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 8080
#define WIDTH 100
#define HEIGHT 100
#define STATIC_PATH "./static"
#define BUFFER_SIZE 4096

static uint8_t canvas[WIDTH][HEIGHT];
static pthread_mutex_t canvas_mutex;

// 클라이언트 목록 관리 구조체
typedef struct per_session_data {
    struct lws *wsi;
    struct per_session_data *prev;
    struct per_session_data *next;
    unsigned char *buf; // 버퍼 포인터로 변경
    size_t buf_size;    // 버퍼 크기
    size_t len;         // 메시지 길이
} per_session_data_t;

// 연결된 모든 클라이언트의 리스트
static per_session_data_t *clients_list = NULL;
static pthread_mutex_t clients_mutex;

/* 
    filename의 확장자에 따라 MIME TYPE을 반환하는 함수
    HTTP 헤더의 CONTEXT-TYPE을 지정해주기 위해 사용한다.
*/
const char *get_mime_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream"; 

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
        return "text/html";
    if (strcmp(ext, ".css") == 0)
        return "text/css";
    if (strcmp(ext, ".js") == 0)
        return "application/javascript";
    if (strcmp(ext, ".json") == 0)
        return "application/json";
    if (strcmp(ext, ".png") == 0)
        return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(ext, ".gif") == 0)
        return "image/gif";
    if (strcmp(ext, ".svg") == 0)
        return "image/svg+xml";
    if (strcmp(ext, ".ico") == 0)
        return "image/x-icon";
    return "application/octet-stream";

}


int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                         void *user, void *in, size_t len) {
    switch (reason) {
    case LWS_CALLBACK_HTTP: {
        const char *requested_uri = (const char *)in;
        char filepath[512];
        struct stat file_stat;

        // Build the file path
        snprintf(filepath, sizeof(filepath), "%s%s", STATIC_PATH, requested_uri);
        if (strcmp(requested_uri, "/") == 0) {
            snprintf(filepath, sizeof(filepath), "%s/index.html", STATIC_PATH);
        }

        // Check if file exists
        if (stat(filepath, &file_stat) < 0 || S_ISDIR(file_stat.st_mode)) {
            lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, NULL);
            return -1;
        }

        // Open the file
        int fd = open(filepath, O_RDONLY);
        if (fd < 0) {
            lws_return_http_status(wsi, HTTP_STATUS_INTERNAL_SERVER_ERROR, NULL);
            return -1;
        }

        // Send headers
        const char *mime_type = get_mime_type(filepath);
        char headers[256];
        snprintf(headers, sizeof(headers),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %ld\r\n"
                 "Connection: close\r\n\r\n",
                 mime_type, file_stat.st_size);

        lws_write(wsi, (unsigned char *)headers, strlen(headers), LWS_WRITE_HTTP);

        // Send file content
        char buffer[BUFFER_SIZE];
        ssize_t read_size;
        while ((read_size = read(fd, buffer, sizeof(buffer))) > 0) {
            lws_write(wsi, (unsigned char *)buffer, read_size, LWS_WRITE_HTTP);
        }

        close(fd);
        return -1; // Close connection after serving the file
    }
    default:
        break;
    }

    return 0;
}

// 모든 연결된 클라이언트들에게 메시지 브로드캐스트
void broadcast_to_clients(const char *protocol_name, unsigned char *message, size_t len) {
    per_session_data_t *psd;

    pthread_mutex_lock(&clients_mutex);
    for (psd = clients_list; psd != NULL; psd = psd->next) {
        if (strcmp(lws_get_protocol(psd->wsi)->name, protocol_name) == 0) {
            // 버퍼 크기 확인 및 재할당
            if (psd->buf_size < LWS_PRE + len) {
                unsigned char *new_buf = realloc(psd->buf, LWS_PRE + len);
                if (!new_buf) {
                    fprintf(stderr, "Failed to reallocate memory for psd->buf\n");
                    continue;
                }
                psd->buf = new_buf;
                psd->buf_size = LWS_PRE + len;
            }
            // 메시지를 세션 데이터에 저장
            memcpy(&psd->buf[LWS_PRE], message, len);
            psd->len = len;

            // 쓰기 가능한 콜백 예약
            lws_callback_on_writable(psd->wsi);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

static int callback_rplace(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    per_session_data_t *psd = (per_session_data_t *)user;

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: //0
            // 클라이언트 연결됨
            psd->wsi = wsi;
            printf("Client connected\n");

            // 버퍼 할당
            psd->buf_size = LWS_PRE + WIDTH * HEIGHT * 3;
            psd->buf = malloc(psd->buf_size);
            if (!psd->buf) {
                fprintf(stderr, "Failed to allocate memory for psd->buf\n");
                return -1;
            }

            // 클라이언트 리스트에 추가
            pthread_mutex_lock(&clients_mutex);
            psd->next = clients_list;
            psd->prev = NULL;
            if (clients_list)
                clients_list->prev = psd;
            clients_list = psd;
            pthread_mutex_unlock(&clients_mutex);

            // 현재 캔버스 상태 전송
            {
                pthread_mutex_lock(&canvas_mutex);
                size_t init_len = WIDTH * HEIGHT * 3;
                unsigned char *init_msg = malloc(init_len);
                if (!init_msg) {
                    pthread_mutex_unlock(&canvas_mutex);
                    fprintf(stderr, "Failed to allocate memory for init_msg\n");
                    return -1;
                }
                size_t index = 0;
                for (uint8_t y = 0; y < HEIGHT; y++) {
                    for (uint8_t x = 0; x < WIDTH; x++) {
                        init_msg[index++] = x;
                        init_msg[index++] = y;
                        init_msg[index++] = canvas[x][y];
                    }
                }
                pthread_mutex_unlock(&canvas_mutex);

                // 버퍼 크기 확인 및 재할당
                if (psd->buf_size < LWS_PRE + init_len) {
                    unsigned char *new_buf = realloc(psd->buf, LWS_PRE + init_len);
                    if (!new_buf) {
                        fprintf(stderr, "Failed to reallocate memory for psd->buf\n");
                        free(init_msg);
                        return -1;
                    }
                    psd->buf = new_buf;
                    psd->buf_size = LWS_PRE + init_len;
                }

                // 메시지 저장 및 전송 예약
                memcpy(&psd->buf[LWS_PRE], init_msg, init_len);
                psd->len = init_len;
                lws_callback_on_writable(psd->wsi);

                free(init_msg);
            }
            break;

        case LWS_CALLBACK_RECEIVE:
            // 클라이언트로부터 픽셀 정보 수신
            if (len == 3) {
                uint8_t *data = (uint8_t *)in;
                uint8_t x = data[0];
                uint8_t y = data[1];
                uint8_t color = data[2];

                if (x < WIDTH && y < HEIGHT) {
                    pthread_mutex_lock(&canvas_mutex);
                    canvas[x][y] = color;
                    pthread_mutex_unlock(&canvas_mutex);

                    // 변경 사항을 브로드캐스트
                    unsigned char msg[3];
                    msg[0] = x;
                    msg[1] = y;
                    msg[2] = color;

                    broadcast_to_clients("rplace-protocol", msg, 3);
                }
            }
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            if (psd->len > 0) {
                int n = lws_write(wsi, &psd->buf[LWS_PRE], psd->len, LWS_WRITE_BINARY);
                if (n < (int)psd->len) {
                    fprintf(stderr, "ERROR %d writing to socket\n", n);
                    return -1;
                }
                psd->len = 0; // 전송 후 길이 초기화
            }
            break;

        case LWS_CALLBACK_CLOSED:
            // 클라이언트 연결 종료
            printf("Client disconnected\n");

            // 버퍼 해제
            if (psd->buf) {
                free(psd->buf);
                psd->buf = NULL;
            }

            // 클라이언트 리스트에서 제거
            pthread_mutex_lock(&clients_mutex);
            if (psd->prev)
                psd->prev->next = psd->next;
            else
                clients_list = psd->next;
            if (psd->next)
                psd->next->prev = psd->prev;
            pthread_mutex_unlock(&clients_mutex);
            break;

        default:
            break;
    }
    return 0;
}

int main(void) {
    struct lws_context_creation_info info;
    struct lws_context *context;
    struct lws_protocols protocols[] = {
        {
            .name = "http-only",
            .callback = callback_http,
            .per_session_data_size = 0,
            .rx_buffer_size = 0,
        },
        {
            "rplace-protocol",
            callback_rplace,
            sizeof(per_session_data_t),
            1024,
        },
        { NULL, NULL, 0, 0 } // NULL로 끝내야 함
    };

    memset(&info, 0, sizeof(info));
    info.port = PORT;
    info.protocols = protocols;

    pthread_mutex_init(&canvas_mutex, NULL);
    pthread_mutex_init(&clients_mutex, NULL);

    // 캔버스 초기화 (필요한 경우)
    memset(canvas, 0, sizeof(canvas));

    context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "lws_create_context failed\n");
        return -1;
    }

    printf("Server is running on http://localhost:%d\n", PORT);

    // 메인 서버 루프
    while (1) { lws_service(context, 1000); }

    pthread_mutex_destroy(&canvas_mutex);
    pthread_mutex_destroy(&clients_mutex);
    lws_context_destroy(context);

    return 0;
}
