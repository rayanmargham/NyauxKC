#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdint.h>
#include <term/term.h>
#include <utils/libc.h>
extern volatile struct limine_module_request modules;
void populate_tmpfs_from_tar();

struct tar_header {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char filesize_octal[12];
  char unix_time_format[12];
  char chksum[8];
  char type[1];
  char name_linked_file[100];
  char ustar[6];
  char ver_ustar[2];
  char owner_name[32];
  char group_name[32];
  char device_major[8];
  char device_minor[8];
  char filenameprefix[155];
} __attribute__((packed));
#ifdef __cplusplus
}
#endif