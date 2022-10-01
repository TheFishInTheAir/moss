#include <stdio.h>
#include <moss.h>
#include <startup.h>

void start_cpu1()
{
    while(1){};
//    moss_portlvl_init();
}


// 
void start_cpu0()
{
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
