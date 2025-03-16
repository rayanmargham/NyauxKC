#include "elf.h"
#include "mem/vmm.h"
#include <fs/vfs/vfs.h>
#include <limine.h>
#include <mem/kmem.h>
#include <stdint.h>

Elf64_Ehdr *get_kernel_elfheader() {
  return (Elf64_Ehdr *)kernelfile.response->executable_file->address;
}
uint64_t get_kerneL_address() {
  return (uint64_t)kernelfile.response->executable_file->address;
}
// function that loads virtual address into memroy from pt_load
void load_pt_loadseg(pagemap *usrmap, uint64_t elfbase, Elf64_Phdr *phdr) {
  void *data =
      uvmm_region_alloc_fixed(usrmap, phdr->p_vaddr, phdr->p_memsz, true);
  if (data == NULL) {
    panic("could not allocate elf\r\n");
  }
  memcpy(data, (void *)(elfbase + phdr->p_offset), phdr->p_filesz);
}
void load_elf_static(pagemap *usrmap, char *path) {
  struct vnode *node = NULL;
  vfs_lookup(NULL, path, &node);
  assert(node != NULL);
  size_t sizeofelf = node->stat.size;
  void *buffer = kmalloc(sizeofelf);
  Elf64_Ehdr *hdr = buffer;
  node->ops->rw(node, 0, sizeofelf, buffer, 0);
  kprintf("load_elf_static(): found elf signature %c%c%c%c\r\n",
          hdr->e_ident[0], hdr->e_ident[1], hdr->e_ident[2], hdr->e_ident[3]);
  for (int i = 0; i < hdr->e_phnum; i++) {
    Elf64_Phdr *phdr = (Elf64_Phdr *)(((uint64_t)buffer) +
                                      (hdr->e_phoff + (i * hdr->e_phentsize)));
    kprintf("program header type %d\r\n", phdr->p_type);
    switch (phdr->p_type) {
    case 1:

      load_pt_loadseg(usrmap, (uint64_t)buffer, phdr);
      kprintf("loaded segment of elf\r\n");
      break;
    default:
      break;
    }
  }
}
void test_elf() { load_elf_static(&ker_map, "/usr/bin/bash"); }