#include <stdio.h>
#include "chaOS.h"



void start_cpu0()
{

    start_cpu0_default();
    //Override
    func();
    
}

void func(void)
{
    printf("Do the thing!!\n");
}

void lol(void)
{
    printf("lol\n");
}