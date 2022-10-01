#include "sdkconfig.h"

#include <interrupt.h>

#include <esp_private/esp_clk.h>
#include <esp_private/esp_int_wdt.h>


#include <stdint.h>

// RAW DOG THE INTERRUPTS!

static volatile uint32_t _this_some_crazy_shit = 0;

// This is a temporary interrupt handler
// This is the slower option, It's implemented but theres no option to enable this on FreeRTOS
IRAM_ATTR void __moss_tick_handler(void *arg)
{
    _this_some_crazy_shit++;
}


void _moss_interrupt_tick_timer_init()
{
   
    unsigned cpuid = 0;
    assert(cpuid==0); // lol
    
    /* Systimer HAL layer object */
    //static systimer_hal_context_t systimer_hal;
    
    /* set system timer interrupt vector */
    
}

uint32_t get_num_ticks()
{
    return _this_some_crazy_shit;
}

void moss_interrupt_init()
{
    // Setup Interrupt Watchdog
    esp_int_wdt_init();

    // Setup Tick Timer Interrupt
    //__moss_interrupt_tick_timer_init();
 

    
}
