#include <stdio.h>
#include <moss.h>
#include <scheduler.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <synch.h>
#include <matrix.h>
#include <contextswitch.h>

 

void app_main(void)
 {
    _moss_log("Current Stack Ptr addr %#08X  TOS: %#08X SP: %#08X\n", moss_sp(), moss_current_process()->top_of_stack, moss_current_process()->stack);

    char version_buf[256];
    moss_version(version_buf, 256);
    _moss_log("Kernel Version: %s\n", version_buf);
    
    matrix_test_run_suite();
    context_switching_test();
    

    while(1);
}
