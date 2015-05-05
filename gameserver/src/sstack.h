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

// dep stack
#define DSTACK_SIZE 32
typedef struct dstack {
    int arr[DSTACK_SIZE];
    // next free index
    int idx;
} dstack;

void dstack_init(dstack *s);
int dstack_contains(dstack *s, int key);
int dstack_empty(dstack *s);
int dstack_size(dstack *s);
void dstack_push(dstack *s, int key);
int dstack_pop(dstack *s);
int dstack_peek(dstack *s);

#endif // SSTACK_H
