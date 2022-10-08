#include <stdio.h>
#include <moss.h>
#include <scheduler.h>
#include <esp_timer.h>


#define PROFILING_PROCESS_LOOP_N (100)
int testing_thing = 0;
volatile int complete = 0;

void profiling_process()
{
    int process_id = moss_active_process->pid;
    for(int i = 0; i < PROFILING_PROCESS_LOOP_N; i++)
    {
        testing_thing = i%(10);
        moss_yield();
    }
    complete++;

    while(1){moss_yield();}
}

#define NUM_PROCS 6
void context_switching_test()
{
    int64_t fstart = esp_timer_get_time();
    printf("Start Time: %lld\n", fstart);


    for(int i = 0; i < NUM_PROCS; i++)
    {
        for(int j = 0; j < PROFILING_PROCESS_LOOP_N; j++)
        {
            testing_thing = j%(10);
        }
    }

    int64_t fend = esp_timer_get_time();
    printf("End Time: %lld\n", fend);
    printf("Diff Time: %lld\n", fend-fstart);



    int64_t pstart = esp_timer_get_time();
    printf("Start Time: %lld\n", pstart);

    for(int i = 0; i < NUM_PROCS; i++)
    {
        moss_instantiate_proc(moss_sched(), NULL, "profiling_proc", profiling_process);
    }

    while(complete<NUM_PROCS)
        moss_yield();

    int64_t pend = esp_timer_get_time();

    printf("End Time: %lld\n", pend);
    printf("Diff Time: %lld\n", pend-pstart);

    int64_t overhead = (pend-pstart)-(fend-fstart);
    printf("Context Overhead: %lld\n", overhead);

    printf("Context Switches: %d\n", (NUM_PROCS+1)*PROFILING_PROCESS_LOOP_N);
    printf("Switch Overhead Avg: %f\n", (double)overhead/(double)((NUM_PROCS+1)*PROFILING_PROCESS_LOOP_N));

}



void app_main(void)
{
    printf("Hello world!\n");

    printf("Impressive we finally got here tbh\n");


    char version_buf[256];
    moss_version(version_buf, 256);
    printf("Kernel Version: %s\n", version_buf);

    context_switching_test();

    while(1);
}
