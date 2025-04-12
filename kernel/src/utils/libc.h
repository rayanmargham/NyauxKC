#pragma once
#include <stddef.h>
#include <stdint.h>
char *strtok(char *restrict s, const char *restrict sep);
char *strdup(const char *str);
size_t strlen(const char *s);
struct ring_buf {
  uint64_t *buf;
  size_t size;
  size_t write_idx;
  size_t read_idx;
};
int get_ringbuf(struct ring_buf *buf, uint64_t *value);
int put_ringbuf(struct ring_buf *buf, uint64_t data);
bool empty_ringbuf(struct ring_buf *buf);
void resize_ringbuf(struct ring_buf *buf, size_t resize);
struct ring_buf *init_ringbuf(size_t thesize);