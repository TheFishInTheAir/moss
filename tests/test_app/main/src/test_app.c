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

    char version_buf[256];
    moss_version(version_buf, 256);
    _moss_log("Kernel Version: %s\n", version_buf);

    context_switching_test();
    //matrix_test_run_suite();

    while(1);
}
