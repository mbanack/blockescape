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
    // dir refers to the blocker of depth 0 in this chain
    // ie for depth > 0 keep the same block direction as depth 0
    //   ie ID_P is blocked RIGHT by id8, sub id2, sub id4
    uint8_t dir;
    // 0 is "immediate blocker"
    uint8_t depth;
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

typedef struct solve_result {
    int solved;
    int moves;
    int num_pieces;
} solve_result;

// boardstate/hash
//   8 bits per square * 36 => 288 bits (9x32)
//   each square is set to the id of its piece (or ID_BLANK)
// disk flag is set to 1 once this bs is written to a puzzle file
typedef struct bsref {
    uint8_t s[36];
    solve_result sr;
    int disk;
} bsref;

typedef struct solved_bs {
    bsref bs;
    solve_result sr;
} solved_bs;

int bsref_equal(bsref *a, bsref *b);
int is_topleft(bsref *, int);
void bsref_clone(bsref *bsb, bsref *bsa);
void bsref_print(bsref *a);
void make_move(bsref *bs, int id, int old_x, int old_y, int new_x, int new_y);
void make_move_dir(bsref *bs, int id, int dir);
int solved(bsref *bs);

int find_piece(bsref *bs, int id, int *x_out, int *y_out, int *bidx_out);
int is_horiz(bsref *bs, int idx);
int is_solvable(bsref *init);

void ai_solve(bsref *init, solve_result *r_out);
void heuristics(bsref *bs, int *dir_out, int *id_out);
void clear_depgraph(depgraph *ss);
void clear_bsref(bsref *h);
void insert_piece(bsref *bs, int id, int width, int height, int new_x, int new_y);
void fill_full_depgraph(bsref *bs, depgraph *ss);
int on_depgraph(bsref *bs, depgraph *ss, node *curnode, node *newnode);

int get_hint(bsref *bs, int *moved_id, int *dir);

#endif // SOLVER_H
