#include "ustar.h"

#include <stdint.h>

#include "fs/vfs/vfs.h"
#include "limine.h"
#include "term/term.h"
#include "utils/basic.h"

// stolen from https://wiki.osdev.org/USTAR :trl:
int oct2bin(unsigned char *str, int size) {
  int n = 0;
  unsigned char *c = str;
  while (size-- > 0) {
    n *= 8;
    n += *c - '0';
    c++;
  }
  return n;
}
void advance(struct tar_header **ptr) {
  *ptr = (void *)*ptr + 512 +
         align_up(oct2bin((unsigned char *)(*ptr)->filesize_octal,
                          strlen((*ptr)->filesize_octal)),
                  512);
}
void populate_tmpfs_from_tar() {
  if (modules.response == NULL) {
    panic("populate_tmpfs_from_tar(): Nyaux Cannot Continue without a "
          "initramfs...\r\n");
  } else if (modules.response->module_count == 0) {
    panic("populate_tmpfs_from_tar(): Nyaux Cannot Continue without a "
          "initramfs...\r\n");
  }
  struct tar_header *ptr = modules.response->modules[0]->address;
  if (strcmp((char *)(ptr->ustar), "ustar") == 0) {
    kprintf("populate_tmpfs_from_tar(): this is a ustar archive, unpacking and "
            "populating the vfs\r\n");
    while ((void *)ptr < (modules.response->modules[0]->address +
                          modules.response->modules[0]->size) -
                             1024) {
      char name[256];
      if (ptr->filenameprefix[0]) {
        npf_snprintf(name, sizeof(name), "%s/%s", ptr->filenameprefix,
                     ptr->name);
      } else {
        npf_snprintf(name, sizeof(name), "%s", ptr->name);
      }
      switch (ptr->type[0]) {
      case '5':
        // kprintf("found directory with path: %s\r\n", ptr->name);
        vfs_create_from_tar(name, VDIR, 0, NULL);
        advance(&ptr);
        break;
      case '0':
        // kprintf("found file with path: %s\r\n", ptr->name);
        uint64_t size = (uint64_t)oct2bin((unsigned char *)ptr->filesize_octal,
                                          strlen(ptr->filesize_octal));
        if (size != 0) {
          vfs_create_from_tar(name, VREG, size, (unsigned char *)ptr + 512);
        } else {
          vfs_create_from_tar(name, VREG, size, NULL);
        }
        // kprintf("size %d\r\n", oct2bin((unsigned char*)ptr->filesize_octal,
        // strlen(ptr->filesize_octal)));

        advance(&ptr);
        break;
      case '1':
        sprintf("ustar(): hard link :c\r\n");
        advance(&ptr);
        break;
      case '2':
        vfs_create_from_tar(name, VSYMLINK, strlen(ptr->name_linked_file),
                            (unsigned char *)ptr->name_linked_file);
        advance(&ptr);
        break;
      default:
        // ptr = (void *)ptr + 512 +
        //       align_up(oct2bin((unsigned char *)ptr->filesize_octal,
        //                        strlen(ptr->filesize_octal)),
        //                512);
        advance(&ptr);
        break;
      }
    }
  } else {
    panic("populate_tmpfs_from_tar(): Invalid tar format...\r\n");
  }
}
