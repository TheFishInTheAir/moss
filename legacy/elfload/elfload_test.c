#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <moss_elfload.h>

int main()
{
    FILE* fd = fopen("prog", "r");
    assert(fd!=NULL);

    fseek(fd, 0L, SEEK_END);
    uint32_t len = ftell(fd);
    rewind(fd);

    int *elf_file = mmap(NULL, len, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fileno(fd), 0);
    assert(elf_file!=NULL);

    moss_process_elf_header(elf_file);
    return 0;
}