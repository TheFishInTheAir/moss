#include <stdio.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos_wifi.h>

#define PROFILING_PROCESS_LOOP_N (100000)
int testing_thing = 0;
volatile int complete = 0;

void profiling_process()
{
    int process_id = 1;
    for(int i = 0; i < PROFILING_PROCESS_LOOP_N; i++)
    {
        testing_thing = i % 10;
        taskYIELD();
    }
    complete++;

    while(1){taskYIELD();}
}

volatile int got_idle = 0;

void core1_idle()
{
    while(xPortGetCoreID()!=1)
    {
        taskYIELD();
    }

    got_idle = 1;
    while(1);
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
            testing_thing = j % 10;
        }
    }

    int64_t fend = esp_timer_get_time();
    printf("End Time: %lld\n", fend);
    printf("Diff Time: %lld\n", fend-fstart);

    xTaskCreate(core1_idle, "idle thing", 126*4, NULL, 1, NULL);

    while(got_idle!=1)
    {
        taskYIELD();
    }

    int64_t pstart = esp_timer_get_time();
    printf("Start Time: %lld\n", pstart);

    TaskHandle_t _handle;
    for(int i = 0; i < NUM_PROCS; i++)
    {
        xTaskCreate(profiling_process, "profiling_proc", 126*4, NULL, 1, &_handle);
    }

    while(complete<NUM_PROCS)
        taskYIELD();

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
    context_switching_test();
    //run_wifi_test();

    while(1);
}
