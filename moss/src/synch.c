#include <synch.h>
#include <moss.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

void moss_semaphore_init(moss_semaphore* semaphore, int start_count)
{
    semaphore->counter = start_count;
    moss_process_exec_queue_init(&semaphore->queue);
    spinlock_initialize(&semaphore->lock);
}

void moss_semaphore_wait(moss_semaphore* semaphore)
{
    spinlock_acquire(&semaphore->lock, SPINLOCK_WAIT_FOREVER);

    semaphore->counter--;

    if(semaphore->counter<0)
    {

        moss_process_exec_queue_push(&semaphore->queue, moss_current_process());
        
        spinlock_release(&semaphore->lock);
        //moss_process_exec_queue_debug_dump(&semaphore->queue);


        // we don't want it to be added to the execution queue again.
        moss_yield_without_queue();
    }
    else
    {
        spinlock_release(&semaphore->lock);
    }

}

void moss_semaphore_signal(moss_semaphore* semaphore)
{

    spinlock_acquire(&semaphore->lock, SPINLOCK_WAIT_FOREVER);
    
    semaphore->counter++;

    if (semaphore->queue.num_elems>0)
    {
        moss_process* proc;
        assert(moss_process_exec_queue_pop(&semaphore->queue, &proc)==MOSS_SUCCESS);
        assert(moss_scheduler_start_proc(moss_sched(), proc)==MOSS_SUCCESS);
    }
    spinlock_release(&semaphore->lock);
}