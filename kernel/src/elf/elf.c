#include "elf.h"

#include "mem/vmm.h"
#include "utils/basic.h"

#include <fs/vfs/vfs.h>
#include <limine.h>
#include <mem/kmem.h>
#include <stdint.h>
#define PT_LOAD 1
#define PT_INTERP 3
#define PT_PHDR 6
#define AT_NULL 0
#define AT_IGNORE 1
#define AT_EXECFD 2
#define AT_PHDR 3
#define AT_PHENT 4
#define AT_PHNUM 5
#define AT_PAGESZ 6
#define AT_BASE 7
#define AT_FLAGS 8
#define AT_ENTRY 9
#define AT_NOTELF 10
#define AT_UID 11
#define AT_EUID 12
#define AT_GID 13
#define AT_EGID 14
Elf64_Ehdr *get_kernel_elfheader() {
  return (Elf64_Ehdr *)kernelfile.response->executable_file->address;
}
uint64_t get_kerneL_address() {
  return (uint64_t)kernelfile.response->executable_file->address;
}
#define FIXEDOFFSET 0x10000
// function that loads virtual address into memroy from pt_load
void load_pt_loadsegpie(uint64_t basefileaddress, uint64_t elfbase,
                        Elf64_Phdr *phdr) {
  // void *data = uvmm_region_alloc_fixed(usrmap, phdr->p_vaddr + FIXEDOFFSET,
  //                                      phdr->p_memsz, false);
  memcpy((void *)(elfbase + phdr->p_vaddr),
         (void *)(basefileaddress + phdr->p_offset), phdr->p_filesz);
  // for things like .bss
  memset((void *)(elfbase + phdr->p_vaddr + phdr->p_filesz), 0,
         (phdr->p_memsz - phdr->p_filesz));
}
void load_elf_pie(pagemap *usrmap, Elf64_Ehdr *hdr, struct ElfInfo *out) {
  void *buffer = (void *)hdr;
  size_t lowest_address = SIZE_MAX;
  size_t highest_address = 0;
  for (int i = 0; i < hdr->e_phnum; i++) {
    Elf64_Phdr *phdr = (Elf64_Phdr *)(((uint64_t)buffer) +
                                      (hdr->e_phoff + (i * hdr->e_phentsize)));
    if (phdr->p_type != PT_LOAD) {
      continue;
    }
    size_t t = phdr->p_vaddr + phdr->p_memsz;
    if (phdr->p_vaddr < lowest_address) {
      lowest_address = phdr->p_vaddr;
    }
    if (t > highest_address) {
      highest_address = t;
    }
  }
  size_t sizeofexecutable = highest_address + lowest_address;
  void *feet = uvmm_region_alloc(usrmap, sizeofexecutable, 0);
  out->entrypoint = (uint64_t)feet - lowest_address + hdr->e_entry;
  out->phent = hdr->e_phentsize;
  out->phnum = hdr->e_phnum;
  for (int i = 0; i < hdr->e_phnum; i++) {
    Elf64_Phdr *phdr = (Elf64_Phdr *)(((uint64_t)buffer) +
                                      (hdr->e_phoff + (i * hdr->e_phentsize)));

    switch (phdr->p_type) {
    case PT_LOAD:

      load_pt_loadsegpie((uint64_t)buffer, ((uint64_t)feet) - lowest_address,
                         phdr);
      kprintf("loaded segment of elf\r\n");
      break;
    case PT_INTERP:
      kprintf("load_elf(): found interp %s\r\n",
              (char *)(buffer + phdr->p_offset));
      out->interpath = strdup((char *)(buffer + phdr->p_offset));

      break;
    case PT_PHDR:
      out->phdr = ((uint64_t)feet - lowest_address) + phdr->p_vaddr;
      break;
    default:
      break;
    }
  }
}
void load_elf(pagemap *usrmap, char *path, char **argv, char **envp,
              struct StackFrame *frame) {
  uint64_t userstack = frame->rsp;
  struct vnode *node = NULL;
  vfs_lookup(NULL, path, &node);
  assert(node != NULL);
  size_t sizeofelf = node->stat.size;
  void *buffer = kmalloc(sizeofelf);
  Elf64_Ehdr *hdr = buffer;
  node->ops->rw(node, 0, sizeofelf, buffer, 0);
  kprintf("load_elf(): found elf signature %c%c%c%c\r\n", hdr->e_ident[0],
          hdr->e_ident[1], hdr->e_ident[2], hdr->e_ident[3]);
  struct ElfInfo info = {};
  load_elf_pie(usrmap, hdr, &info);
  uint64_t entrypoint = info.entrypoint;
  kfree(buffer, sizeofelf);
  if (info.interpath) {
    kprintf("entry point is %lx\r\n", info.entrypoint);
    vfs_lookup(NULL, info.interpath, &node);
    assert(node != NULL);
    sizeofelf = node->stat.size;
    buffer = kmalloc(sizeofelf);
    hdr = buffer;
    node->ops->rw(node, 0, sizeofelf, buffer, 0);
    struct ElfInfo interpinfo = {};
    load_elf_pie(usrmap, hdr, &interpinfo);
    entrypoint = interpinfo.entrypoint;
    kprintf("interp entry point is %lx\r\n", entrypoint);
  }

  int argc = 0;
  while (argv[argc] != NULL) {
    argc++;
  }
  int g = 0;
  while (envp[g] != NULL) {
    g++;
  }
  uint64_t *argv_user = kmalloc(argc * sizeof(uint64_t));
  uint64_t *envp_user = kmalloc(g * sizeof(uint64_t));
  for (int i = 0; i < argc; i++) {
    size_t len = strlen(argv[i]) + 1;
    userstack -= len;
    memcpy((void *)userstack, argv[i], len);
    argv_user[i] = userstack;
  }
  for (int i = 0; i < g; i++) {
    size_t len = strlen(envp[i]) + 1;
    userstack -= len;
    memcpy((void *)userstack, envp[i], len);
    envp_user[i] = userstack;
  }
  userstack = align_down(userstack, 16);
  uint64_t auxv[4][2] = {{AT_ENTRY, info.entrypoint},
                         {AT_PHDR, info.phdr},
                         {AT_PHENT, info.phent},
                         {AT_PHNUM, info.phnum}};
  // reasons why its + 4 is because in the sysv abi theres a null auxv
  // 2 0 eight byte each and a arugment count
  uint64_t pointer_count = (sizeof(auxv) / sizeof(uint64_t)) + argc + g + 4;
  uint64_t byte_count = pointer_count * sizeof(uint64_t);
  if (!is_aligned(userstack - byte_count, 16)) {
    userstack -= 8;
  }
  uint64_t *stack = (uint64_t *)userstack;
  *--stack = AT_NULL;
  for (int i = 0; i < 4; i++) {
    *--stack = auxv[i][1];
    *--stack = auxv[i][0];
  }
  *--stack = 0;
  // push enviorment pointers
  for (int i = 0; i < g; i++) {
    *--stack = envp_user[g - i - 1];
  }
  *--stack = 0;
  // push arugment array
  for (int i = 0; i < argc; i++) {
    *--stack = argv_user[argc - i - 1];
  }
  // arugment count
  *--stack = argc;
  frame->rsp = (uint64_t)stack;
  frame->rip = entrypoint;
}
