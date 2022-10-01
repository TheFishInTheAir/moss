#include <stdio.h>
#include <moss.h>

void app_main(void)
{
    printf("Hello world!\n");

    printf("Impressive we finally got here tbh\n");


    char version_buf[256];
    moss_version(version_buf, 256);
    printf("Kernel Version: %s", version_buf);
}
