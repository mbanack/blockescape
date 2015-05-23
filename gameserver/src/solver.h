/*
THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY
APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT
HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY
OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM
IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF
ALL NECESSARY SERVICING, REPAIR OR CORRECTION.
IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS
THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY
GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE
USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF
DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD
PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS),
EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGES.
*/
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
    bsref operator=(const bsref &rhs){
        if(&rhs == this ) return *this;
        for(int i = 0; i < 36; ++i) s[i] = rhs.s[i];
        return *this;
    }
} bsref;

int bsref_equal(bsref *a, bsref *b);
int is_topleft(bsref *, int);
void bsref_clone(bsref *bsb, bsref *bsa);

int is_solvable(bsref *init);

typedef struct solve_result {
    int solved;
    int moves;
    solve_result operator=(const solve_result &rhs){
        if(&rhs == this) return *this;
        solved = rhs.solved; moves = rhs.moves;
        return *this;
    }
} solve_result;

void ai_solve(bsref *init, solve_result *r_out);

#endif // SOLVER_H
