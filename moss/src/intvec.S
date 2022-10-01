#include <moss.h>

#define CLOCK_DIVISOR


// This is the asm interrupt handler

xt_user_int_timer_tick:
	mov a12, a6 // Not sure what this does entirely
	call0 __moss_tick_handler


.globl  _frxt_tick_timer_init
    .type   _frxt_tick_timer_init,@function
    .align  4
tick_timer_init:
	
    ENTRY(16)


    /* Set up the periodic tick timer (assume enough time to complete init). */
    movi    a2, __tick_divisor
    l32i    a3, a2, 0
    rsr     a2, CCOUNT              /* current cycle count */
    add     a2, a2, a3              /* time of first timer interrupt */
    wsr     a2, XT_CCOMPARE         /* set the comparator */

    /*
    Enable the timer interrupt at the device level. Don't write directly
    to the INTENABLE register because it may be virtualized.
    */
    movi    a2, XT_TIMER_INTEN
    call0   xt_ints_on