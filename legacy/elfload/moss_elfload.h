#pragma once
#include <stdint.h>

typedef struct _moss_elf_data
{
    int i;
} moss_elf_data;


int moss_process_elf_header(uint8_t* addr);