#include "sdkconfig.h"

#include <interrupt.h>

#include <esp_private/esp_clk.h>
#include <esp_private/esp_int_wdt.h>
#include <assert.h>

#include <stdint.h>

// NOTE: This is for systick Init. As we aren't doing preemptive scheduling
//       this hasn't been worked on at all.

void _moss_interrupt_tick_timer_init()
{
   
    /* Systimer HAL layer object */
    //static systimer_hal_context_t systimer_hal;
    
    /* set system timer interrupt vector */
    
}

void moss_interrupt_init()
{
    // Setup Interrupt Watchdog
    esp_int_wdt_init();

    // Setup Tick Timer Interrupt
    //__moss_interrupt_tick_timer_init();
 

    
}
