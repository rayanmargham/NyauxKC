#include "utils/libc.h"
#include <limits.h>
#include <mem/kmem.h>
#include <stddef.h>
#include <stdint.h>

// ----------------------------------------------------------------------
// Copyright © 2005-2014 Rich Felker, et al.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ----------------------------------------------------------------------

#define ALIGN (sizeof(size_t))
#define ONES ((size_t)-1 / UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(x) ((x) - ONES & ~(x) & HIGHS)

size_t strlen(const char *s) {
  const char *a = s;
#ifdef __GNUC__
  typedef size_t __attribute__((__may_alias__)) word;
  const word *w;
  for (; (uintptr_t)s % ALIGN; s++)
    if (!*s)
      return s - a;
  for (w = (const void *)s; !HASZERO(*w); w++)
    ;
  s = (const void *)w;
#endif
  for (; *s; s++)
    ;
  return s - a;
}
char *strchrnul(const char *s, int c) {
  c = (unsigned char)c;
  if (!c)
    return (char *)s + strlen(s);

#ifdef __GNUC__
  typedef size_t __attribute__((__may_alias__)) word;
  const word *w;
  for (; (uintptr_t)s % ALIGN; s++)
    if (!*s || *(unsigned char *)s == c)
      return (char *)s;
  size_t k = ONES * c;
  for (w = (void *)s; !HASZERO(*w) && !HASZERO(*w ^ k); w++)
    ;
  s = (void *)w;
#endif
  for (; *s && *(unsigned char *)s != c; s++)
    ;
  return (char *)s;
}

#define BITOP(a, b, op)                                                        \
  ((a)[(size_t)(b) / (8 * sizeof *(a))] op(size_t) 1                           \
   << ((size_t)(b) % (8 * sizeof *(a))))

size_t strcspn(const char *s, const char *c) {
  const char *a = s;
  size_t byteset[32 / sizeof(size_t)];

  if (!c[0] || !c[1])
    return strchrnul(s, *c) - a;

  memset(byteset, 0, sizeof byteset);
  for (; *c && BITOP(byteset, *(unsigned char *)c, |=); c++)
    ;
  for (; *s && !BITOP(byteset, *(unsigned char *)s, &); s++)
    ;
  return s - a;
}

size_t strspn(const char *s, const char *c) {
  const char *a = s;
  size_t byteset[32 / sizeof(size_t)] = {0};

  if (!c[0])
    return 0;
  if (!c[1]) {
    for (; *s == *c; s++)
      ;
    return s - a;
  }

  for (; *c && BITOP(byteset, *(unsigned char *)c, |=); c++)
    ;
  for (; *s && BITOP(byteset, *(unsigned char *)s, &); s++)
    ;
  return s - a;
}
char *strtok(char *s, const char *sep) {
  static char *p;
  if (!s && !(s = p))
    return NULL;
  s += strspn(s, sep);
  if (!*s)
    return p = 0;
  p = s + strcspn(s, sep);
  if (*p)
    *p++ = 0;
  else
    p = 0;
  return s;
}
// Copyright (C) 2024-2024 Rayan Margham

// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted.

// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
// OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

char *strdup(const char *str) {
  size_t length = strlen(str);
  char *new_string = kmalloc(length + 1);
  memcpy(new_string, str, length);
  new_string[length] = '\0';
  return new_string;
}
struct ring_buf *init_ringbuf(size_t thesize) {
  struct ring_buf *ringbuf = kmalloc(sizeof(struct ring_buf));
  ringbuf->size = thesize;
  ringbuf->write_idx = 0;
  ringbuf->read_idx = 0;
  ringbuf->buf = kmalloc(align_up(thesize * sizeof(uint64_t), 8));
  return ringbuf;
}
// referenced from wikipedia but i sorta undestand it
int put_ringbuf(struct ring_buf *buf, uint64_t data) {
  assert(buf != NULL || buf->buf != NULL || buf->size != 0);
  if ((buf->write_idx + 1) % buf->size == buf->read_idx) {

    return 0;
  }
  buf->buf[buf->write_idx] = data;
  buf->write_idx = (buf->write_idx + 1) % buf->size;
  return 1;
}
int get_without_consumeringbuf(struct ring_buf *buf, uint64_t *value) {
  if (buf->read_idx == buf->write_idx) {
    return 0;
  }
  assert(buf != NULL || buf->buf != NULL);
  *value = buf->buf[buf->read_idx];
  return 1;
}
int get_ringbuf(struct ring_buf *buf, uint64_t *value) {
  if (buf->read_idx == buf->write_idx) {
    return 0;
  }
  assert(buf != NULL || buf->buf != NULL);
  *value = buf->buf[buf->read_idx];
  buf->read_idx = (buf->read_idx + 1) % buf->size;
  return 1;
}
bool empty_ringbuf(struct ring_buf *buf) {
  if (buf->read_idx == buf->write_idx) {
    return true;
  } else {
    return false;
  }
}
void resize_ringbuf(struct ring_buf *buf, size_t resize) {
  assert(buf != NULL || buf->buf != NULL);
  void *new = kmalloc(resize);
  void *old = buf->buf;
  memcpy(new, old, buf->size);
  kfree(old, buf->size);
  buf->size = resize;
  buf->buf = new;
}
size_t ringbuf_size(struct ring_buf *buf) { return buf->size; }

size_t find_second_component_of_path(const char *path, char **out) {
  const char *keep = path;
  size_t funkybeat = 0;
  while (*path) {
    if (*path == '/') {
      funkybeat += 1; // don't forget, im with you in the dark
      path += 1;
      continue;
    }
    const char *start = path;
    while (*path && *path != '/') {
      path += 1;
    }
    size_t len = path - start;
    if (len == 0) {
      continue;
    }
    funkybeat += len;
    const char *check = start + len + 1;
    while (*check && *check != '/') {
      check += 1;
    }
    if ((*check == '/' && *(check + 1) == '\0') || *(check) == '\0') {
      char *bigballs = kmalloc(funkybeat + 1);
      memcpy(bigballs, keep, funkybeat);
      bigballs[funkybeat] = '\0';
      *out = bigballs;
      return funkybeat + 1; // now it is up to the user to kfree this shit
    }
  }
  return 0;
}
size_t find_last_component_of_path(const char *path, char **out) {
  while (*path) {
    if (*path == '/') {
      path += 1;
      continue;
    }
    const char *start = path;
    while (*path && *path != '/') {
      path += 1;
    }
    size_t len = path - start;
    if (len == 0) {
      continue;
    }
    char *component = kmalloc(len + 1);
    memcpy(component, start, len);
    component[len] = 0;
    if (!*path) {
      *out = component;
      return len;
    }
    kfree(component, len + 1);
  }
  return 0;
}