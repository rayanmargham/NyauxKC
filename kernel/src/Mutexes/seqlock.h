#pragma once
#include <stdint.h>
#include <stddef.h>
#include <term/term.h>
struct seq_lock {
    size_t version;
};
void seq_read(struct seq_lock *lock, void (*read)(void *data, void *variable), void *data, void *variable);
void seq_write_mutiwriters(struct seq_lock *lock, void (*write)(void *data, void *variable), void *data, void *variable);
void seq_write(struct seq_lock *lock, void (*write)(void *variable, void *data), void *data, void *variable);