#pragma once
#include <scheduler.h>
#include <spinlock.h>

typedef struct _moss_semaphore
{
    volatile int counter;

    moss_process_exec_queue queue;
    spinlock_t lock;

} moss_semaphore;

void moss_semaphore_init(moss_semaphore*, int start_count);

void moss_semaphore_wait(moss_semaphore*);
void moss_semaphore_signal(moss_semaphore*);