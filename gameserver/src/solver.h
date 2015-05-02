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
    blocker blockers[NUM_BLOCKERS];
} node;

typedef struct solvestate {
    // map[PIECE_IDX] = tree node
    node map[36];
} solvestate;

// boardstate/hash
//   8 bits per square * 36 => 288 bits (9x32)
//   each square is set to the id of its piece (or ID_BLANK)
typedef uint8_t bstate[36];

typedef struct bsref {
    bstate s;
} bsref;

int is_topleft(bsref, int);

#endif // SOLVER_H
