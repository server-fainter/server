// File: http_server.c
#include <libwebsockets.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define PORT 8080
#define STATIC_PATH "./static"
#define BUFFER_SIZE 4096

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

int main() {
    struct lws_context_creation_info info;
    struct lws_context *context;
    struct lws_protocols protocols[] = {
        {
            .name = "http-only",
            .callback = callback_http,
            .per_session_data_size = 0,
            .rx_buffer_size = 0,
        },
        {NULL, NULL, 0, 0} 
    };

    memset(&info, 0, sizeof info);
    info.iface = "127.0.0.1";       // 특정 IP로 바인딩(명시하지 않으면 로컬 호스트)
    info.port = PORT;
    info.protocols = protocols;

    // Create the context
    context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "Failed to create lws context\n");
        return 1;
    }

    printf("Server is running on http://localhost:%d\n", PORT);

    // Run the event loop
    while (lws_service(context, 0) >= 0);

    lws_context_destroy(context);
    return 0;
}
