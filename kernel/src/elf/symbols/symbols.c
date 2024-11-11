
#include "symbols.h"
#include "term/term.h"
#include <stdint.h>
void get_symbols() {
  Elf64_Ehdr *hdr = get_kernel_elfheader();
  kprintf("get_symbols(): found elf signature %c%c%c%c\n", hdr->e_ident[0],
          hdr->e_ident[1], hdr->e_ident[2], hdr->e_ident[3]);
  Elf64_Shdr **sections = ((Elf64_Shdr **)(uint64_t)hdr + hdr->e_shoff);
  kprintf("get_symbols(): found %lu symbols\n", hdr->e_shnum);
  for (int i = 0; i < hdr->e_shnum; i++) {
    Elf64_Shdr *oh = sections[i];
    if (oh->sh_type == 2) {
      kprintf("found linker symbol table\n");
    }
  }
}