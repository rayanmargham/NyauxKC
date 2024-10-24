#pragma once
typedef enum {
  ERROR,
  CHAR,
  STRING,
  POINTER,
} OKOR;

typedef union {
  char c;
  char *string;
  void *ptr;
} Value;

typedef struct {
  OKOR kind;
  Value value;
} kcresult;