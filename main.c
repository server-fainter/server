#include "context.h"
#include "task_queue.h"
#include "thread_pool.h"
#include "server.h"
#include "clientmanager.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>

int main() {
    
    Context ctx;
    init_server(&ctx.sm); 
    init_clients(&ctx.cm); 
    init_task_queue(&ctx.queue);

    ThreadPool pool;
    init_thread_pool(&pool, &ctx);
    
    // epoll을 통해 이벤트 감지 및 처리
    while (1) {
        int num_events = epoll_wait(ctx.sm.epoll_fd, ctx.sm.events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait failed");
            continue;
        }

        for (int i = 0; i < num_events; i++) {
            if (ctx.sm.events[i].data.fd == ctx.sm.server_socket) 
            {
                // Accpet 작업 푸쉬
                Task task = {ctx.sm.server_socket, 0};
                push_task(&ctx.queue, task);
            }
            else 
            {
                // 클라이언트 요청 작업 푸시
                Task task = {ctx.sm.events[i].data.fd, 1};
                push_task(&ctx.queue, task);
            }
        }
    }

    close_server(&ctx.sm);
    destroy_thread_pool(&pool);

    return 0;
}
