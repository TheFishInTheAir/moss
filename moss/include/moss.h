#pragma once
#include <stdint.h>
#include <spinlock.h>

// TODO: put this into a config file
#define DEBUG



#define MOSS_SUCCESS   1
#define MOSS_FAIL      0

#define MOSS_INIT_CONST 7

#define MOSS_VERSION_MAJOR 0
#define MOSS_VERSION_MINOR 1

#define XT_CLOCK_FREQ       
#define XT_TICK_DIVISOR     (XT_CLOCK_FREQ / XT_TICK_PER_SEC)

// TODO: set up a linter for unhandled error returns
#define PROP(x) {int __r = x; if(__r<=0) return __r;}

typedef uint32_t moss_address;

void func(void);

void moss_version(char* buf, uint32_t size);

int moss_core_id();
void _moss_nop();
void moss_interrupt_yield();
void _moss_xt_dispatch();

void _moss_log(const char*, ...);
void _moss_log_acquire();
void _moss_log_release();

extern void moss_ctx_switch();