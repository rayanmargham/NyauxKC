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
#define ONES ((size_t) - 1 / UCHAR_MAX)
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
char *strtok(char *restrict s, const char *restrict sep) {
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
char *strdup(const char *str) {
  size_t length = strlen(str);
  char *new_string = kmalloc(length + 1);
  memcpy(new_string, str, length);
  new_string[length] = '\0';
  return new_string;
}
