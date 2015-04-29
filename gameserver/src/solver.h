// solver.h
// blockescape

#ifndef SOLVER_H
#define SOLVER_H


typedef struct node {
    uint8_t id;
    uint8_t init;
    // node ids blocking left and right respectively
    uint8_t l[5];
    uint8_t r[5];
} node;

typedef struct boardstate {
    // 2 lists, mapping ids and type
    // (or bitmasks)
    uint8_t id[36];
    uint8_t type[36];
} boardstate;

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
