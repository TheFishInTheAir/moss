#include <stdio.h>
#include <moss.h>
#include <startup.h>
#include <esp_log.h>
#include <stdarg.h>
#include <spinlock.h>
#include <scheduler.h>


static spinlock_t slock;

void start_cpu_other_cores()
{

    moss_cpu1_init();
}

void start_cpu0()
{
    spinlock_initialize(&slock);
    moss_portlvl_init();
}

void moss_version(char* buf, uint32_t size)
{
    uint32_t real_size = snprintf(buf, size,
				  "moss kernel Version %d.%dDEV",
				  MOSS_VERSION_MAJOR,
				  MOSS_VERSION_MINOR);
    assert(real_size < size && "Given Buffer Size Too Small for Kernel Version Text");
}

void _moss_log_acquire()
{
    assert(spinlock_acquire(&slock, SPINLOCK_WAIT_FOREVER));
}

void _moss_log_release()
{
    spinlock_release(&slock);
}


// This is a temporary logging helper. Needed some synchronization for serial ownership
// Eventually add 
void _moss_log(const char* format, ...)
{
    assert(spinlock_acquire(&slock, SPINLOCK_WAIT_FOREVER));
    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);

    spinlock_release(&slock);
}