#include <moss_elfload.h>
#include <elf.h>
#include <moss_net.h>
#include <stdio.h>

int moss_process_elf_header(uint8_t* addr)
{
    Elf32_Ehdr* header = (Elf32_Ehdr*) addr;
    if(header->e_ident[1]!='E' ||
        header->e_ident[2]!='L' || 
        header->e_ident[3]!='F')
    {
        //TODO: standardize logging
        printf("Invalid ELF Header\n");
        return MOSS_FAIL;
    }

    printf("Valid ELF Header\n");

    Elf32_Shdr* sheader = (Elf32_Shdr*) (addr+header->e_shoff);


    for(int i = 0; i < header->e_shnum; i++)
    {
        printf("Woooo Section #%d Name: %u\n", i, sheader[i].sh_name);
    }

    return MOSS_SUCCESS;
}