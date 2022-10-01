#pragma once
#include <stdint.h>

#define MOSS_SUCCESS   1
#define MOSS_FAIL      0

#define MOSS_INIT_CONST 7

#define MOSS_VERSION_MAJOR 0
#define MOSS_VERSION_MINOR 1

#define XT_CLOCK_FREQ       
#define XT_TICK_DIVISOR     (XT_CLOCK_FREQ / XT_TICK_PER_SEC)

// TODO: make a linter for unhandled error returns (do a rust-y kinda thing)
#define PROP(x) {int __r = x; if(__r<=0) return __r;}

void func(void);

void moss_version(char* buf, uint32_t size);
