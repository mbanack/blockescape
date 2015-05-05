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

// dependency graph of blockers
typedef struct depgraph {
    // map[PIECE_IDX] = tree node
    node map[36];
} depgraph;

// boardstate/hash
//   8 bits per square * 36 => 288 bits (9x32)
//   each square is set to the id of its piece (or ID_BLANK)
typedef struct bsref {
    uint8_t s[36];
} bsref;

int bsref_equal(bsref *a, bsref *b);
int is_topleft(bsref *, int);
void bsref_clone(bsref *bsb, bsref *bsa);

#endif // SOLVER_H
