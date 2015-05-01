// solver.h
// blockescape

#ifndef SOLVER_H
#define SOLVER_H

typedef struct blocker {
    uint8_t id;
    uint8_t dir;
} blocker;

#define NUM_BLOCKERS 20

typedef struct node {
    uint8_t id;
    uint8_t init;
    blocker block[NUM_BLOCKERS];
} node;

typedef struct boardstate {
    // 2 lists, mapping ids and type
    // (or bitmasks)
    uint8_t id[36];
    // TODO: I don't ever use type... what is it?
    //   do they still all move normally?
    uint8_t type[36];
} boardstate;

int is_topleft(boardstate *, int);

typedef struct hash {
    // 4 bits per square * 36 => 144 bits
    // a square is only set if it is the top-left position of
    //   a piece, and is set to the id of that piece
    // so with 6 columns, each u32 is an entire row
    uint32_t b[5];
} hash;

typedef struct solvestate {
    // map[PIECE_IDX] = tree node
    node map[36];
} solvestate;

#endif // SOLVER_H
