// sstack.h
// solver stack implementation
//   keyed by board hash (*bsref)

#ifndef SSTACK_H
#define SSTACK_H

#define SSTACK_SIZE 1024

// solver stack
typedef struct sstack {
    bsref arr[SSTACK_SIZE];
    // next free index
    int idx;
} sstack;

void sstack_init(sstack *s);
int sstack_contains(sstack *s, bsref *key);
int sstack_empty(sstack *s);
int sstack_size(sstack *s);
void sstack_push(sstack *s, bsref *key);
void sstack_pop(sstack *s, bsref *key_out);
void sstack_peek(sstack *s, bsref *key_out);

#endif // SSTACK_H
