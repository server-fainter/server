#ifndef CONTEXT_H
#define CONTEXT_H   

#include "server.h"
#include "clientmanager.h"
#include "task_queue.h"

typedef struct {
    ServerManager sm;
    ClientManager cm;
    TaskQueue queue;
} Context;

#endif // MAIN_H
