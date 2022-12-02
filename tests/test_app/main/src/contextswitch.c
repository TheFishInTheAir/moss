#include <contextswitch.h>
#include <stdio.h>
#include <moss.h>
#include <scheduler.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <synch.h>
#include <matrix.h>



#define PROFILING_PROCESS_LOOP_N (100000)
int testing_thing = 0;
volatile int complete = 0;

static moss_semaphore lock;
static moss_semaphore completion;

volatile static int magic = 1;

void profiling_process()
{
    assert(moss_core_id()==0);
    for(int i = 0; i < PROFILING_PROCESS_LOOP_N; i++)
    {
        testing_thing = i % 10;
        moss_yield();
    }

    moss_semaphore_wait(&lock);
    complete++;
    moss_semaphore_signal(&lock);

    moss_semaphore_signal(&completion);

    moss_terminate();
}

// This is temporary, should add a way to suspend individual cores.
void hang_cpu1()
{
    while(moss_core_id()!=1)
    {
        moss_yield();
    }
    _moss_log("Holding up cpu1\n");
    while(1)
    {
        _moss_nop();
    }
}

#define NUM_PROCS 10
void context_switching_test()
{
    moss_semaphore_init(&lock, 1);
    moss_semaphore_init(&completion, 0);

    moss_instantiate_proc(moss_sched(), NULL, "hang_cpu1", hang_cpu1, NULL);

    int64_t fstart = esp_timer_get_time();
    _moss_log("Start Time: %lld\n", fstart);

    for(int i = 0; i < NUM_PROCS; i++)
    {
        for(int j = 0; j < PROFILING_PROCESS_LOOP_N; j++)
        {
            testing_thing = j % 10;
        }
    }

    int64_t fend = esp_timer_get_time();
    _moss_log("End Time: %lld\n", fend);
    _moss_log("Diff Time: %lld\n", fend-fstart);

    int64_t start_switches = moss_get_context_switches();

    //moss_semaphore_wait(&lock);

    int64_t pstart = esp_timer_get_time();
    _moss_log("Start Time: %lld\n", pstart);

    for(int i = 0; i < NUM_PROCS; i++)
    {
        char id[16] = "profiling_task0";
        id[14] = i+48;
        _moss_log("Making new proc '%s'\n", id);

        moss_instantiate_proc(moss_sched(), NULL, id, profiling_process, NULL);
    }

    // Back of queue
    while(complete!=NUM_PROCS)
    {        
        moss_semaphore_wait(&completion);
        _moss_log("Task completed\n");
    }
    moss_process_exec_queue_debug_dump(&moss_sched()->queue);

    int64_t pend = esp_timer_get_time();

    int64_t end_switches = (int64_t) moss_get_context_switches();
    _moss_log("End Time: %lld\n", pend);
    _moss_log("Diff Time: %lld\n", pend-pstart);

    int64_t overhead = (pend-pstart)-(fend-fstart);
    _moss_log("Context Overhead: %lld\n", overhead);

    _moss_log("Context Switches: %lld\n", end_switches-start_switches);


    // Using Printf for float formatting
    printf("Switch Overhead Avg: %f\n", (double)overhead/(double)(end_switches-start_switches));
    
}
