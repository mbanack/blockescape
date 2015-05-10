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
#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "time.h"

#include "solver.h"
#include "sstack.h"

// run as "g++ -o solver solver.c && ./solver" from src/ dir

#define NUM_GEN 10
// the minimum number of moves to solve the puzzle
#define MIN_MOVES 1
#define SHOW_MOVES 1

using namespace std;

// board is 6x6

// space reqs at end:
//    board_history will cap out at 1024 or something arbitrary (max solve len)
//    seen could grow rather large.
//      we may be able to analyze seen "hits" and try to throw out
//        the ones that heuristically never hit

#define ID_BLANK 0x00
#define ID_P 0x01
#define ID_MAX 0x15
#define ID_NUM 0x16

#define RIGHT 1
#define LEFT  0
#define UP    2
#define DOWN  3
#define NULL_DIR -1

// convert board idx in 0 .. 35 to x, y in 0 .. 5
#define BIDX_TO_X(bidx) ((bidx) % 6)
#define BIDX_TO_Y(bidx) ((bidx) / 6)
// convert x,y to board idx
#define XY_TO_BIDX(x, y) ((6 * (y) + (x)))

bsref null_bstate;
bsref board_init;
sstack seen;

// should we consider free moves of other pieces such as the player piece
const int HEURISTIC_CREEP = 0;

void sstack_init(sstack *s) {
    s->idx = 0;
    for (int i = 0; i < SSTACK_SIZE; i++) {
        memset(&s->arr[i], 0x00, sizeof(bsref));
    }
}

int sstack_contains(sstack *s, bsref *key) {
    for (int i = 0; i < s->idx; i++) {
        if (bsref_equal(key, &s->arr[i])) {
            return 1;
        }
    }
    return 0;
}

int sstack_empty(sstack *s) {
    return s->idx == 0;
}

int sstack_size(sstack *s) {
    return s->idx;
}

void sstack_push(sstack *s, bsref *key) {
    if (s->idx == SSTACK_SIZE) {
        printf("ERROR: max stack size (%d) exceeded.\n", SSTACK_SIZE);
        return;
    }

    bsref_clone(&s->arr[s->idx], key);
    s->idx++;
}

void sstack_pop(sstack *s, bsref *key_out) {
    if (s->idx == 0) {
        printf("ERROR: tried to pop empty stack\n");
        return;
    }

    bsref_clone(key_out, &s->arr[s->idx - 1]);
    s->idx--;
}

void sstack_peek(sstack *s, bsref *key_out) {
    if (s->idx == 0) {
        printf("ERROR: tried to peek empty stack\n");
        return;
    }

    bsref_clone(key_out, &s->arr[s->idx - 1]);
}

void dstack_init(dstack *s) {
    s->idx = 0;
    for (int i = 0; i < DSTACK_SIZE; i++) {
        memset(&s->arr[i], 0x00, sizeof(int));
    }
}

int dstack_contains(dstack *s, int key) {
    for (int i = 0; i < s->idx; i++) {
        if (key == s->arr[i]) {
            return 1;
        }
    }
    return 0;
}

int dstack_empty(dstack *s) {
    return s->idx == 0;
}

int dstack_size(dstack *s) {
    return s->idx;
}

void dstack_push(dstack *s, int key) {
    if (s->idx == DSTACK_SIZE) {
        printf("ERROR: max stack size (%d) exceeded.\n", DSTACK_SIZE);
        return;
    }

    s->arr[s->idx] = key;
    s->idx++;
}

int dstack_pop(dstack *s) {
    if (s->idx == 0) {
        printf("ERROR: tried to pop empty stack\n");
        return -1;
    }

    s->idx--;
    return s->arr[s->idx];
}

int dstack_peek(dstack *s) {
    if (s->idx == 0) {
        printf("ERROR: tried to peek empty stack\n");
        return -1;
    }

    return s->arr[s->idx - 1];
}


uint8_t id_to_hex(int id) {
    if (id < 10) {
        return 48 + id; // decimal conv.
    } else {
        return 55 + id; // hex conv.
    }
}

void print_board(bsref *bs) {
    for (int i = 0; i < 36; i++) {
        printf("%c ", id_to_hex(bs->s[i]));
        if ((i + 1) % 6 == 0) {
            printf("\n");
        }
    }
}

void print_boardhash(bsref *bs) {
    for (int i = 0; i < 36; i++) {
        printf("%c", bs->s[i] + '0');
    }
    printf("\n");
}

void print_depgraph(depgraph *ss) {
    printf("==DG\n");
    for (int i = 0; i < 36; i++) {
        node *n = &ss->map[i];
        if (n->init == 1) {
            printf("node %d\n", n->id);
            for (int i = 0; i < NUM_BLOCKERS; i++) {
                if (n->blockers[i].id != ID_BLANK) {
                    printf("  %d (%d)\n", n->blockers[i].id,
                                          n->blockers[i].dir);
                }
            }
        }
    }
}

void print_seen() {
    printf("[== seen ==]\n");
    for (int i = 0; i < sstack_size(&seen); i++) {
        print_boardhash(&seen.arr[i]);
    }
    printf("[==/seen/==]\n");
}

void insert_piece(bsref *bs, int id, int width, int height, int new_x, int new_y) {
    int bidx = XY_TO_BIDX(new_x, new_y);
    if (width != 1) {
        for (int i = 0; i < width; i++) {
            bs->s[bidx + i] = id;
        }
    } else {
        for (int i = 0; i < height; i++) {
            bs->s[bidx + (6 * i)] = id;
        }
    }
}

int is_horiz(bsref *bs, int idx) {
    int col = idx % 6;
    if (col != 0) {
        if (bs->s[idx - 1] == bs->s[idx]) {
            return 1;
        }
    }
    if (col != 5) {
        if (bs->s[idx + 1] == bs->s[idx]) {
            return 1;
        }
    }
    return 0;
}

// attempts to find a piece with given id, and returns its loc in x,y
void find_piece(bsref *bs, int id, int *x, int *y) {
    *x = -1;
    *y = -1;

    for (int i = 0; i < 36; i++) {
        if (bs->s[i] == id && is_topleft(bs, i)) {
            *x = BIDX_TO_X(i);
            *y = BIDX_TO_Y(i);
            return;
        }
    }
}

int calc_width(bsref *bs, int id) {
    int x, y;
    find_piece(bs, id, &x, &y);
    if (x != -1) {
        int w = 0;
        for (int bidx = XY_TO_BIDX(x, y); bidx < 36; bidx++) {
            if (bs->s[bidx] != id) {
                return w;
            }
            w++;
        }
    }
    return -1;
}

int calc_height(bsref *bs, int id) {
    int x, y;
    find_piece(bs, id, &x, &y);
    if (x != -1) {
        int h = 0;
        for (int bidx = XY_TO_BIDX(x, y); bidx < 36; bidx+=6) {
            if (bs->s[bidx] != id) {
                return h;
            }
            h++;
        }
    }
    return -1;
}

void make_move(bsref *bs, int id, int old_x, int old_y, int new_x, int new_y) {
    int bidx = XY_TO_BIDX(old_x, old_y);
    if (is_horiz(bs, bidx)) {
        int width = calc_width(bs, id);
        insert_piece(bs, ID_BLANK, width, 1, old_x, old_y);
        insert_piece(bs, id, width, 1, new_x, new_y);
    } else {
        int height = calc_height(bs, id);
        insert_piece(bs, ID_BLANK, 1, height, old_x, old_y);
        insert_piece(bs, id, 1, height, new_x, new_y);
    }
    printf("==>\n");
    print_board(bs);
    printf("\n");
}

int is_piece(bsref *bs, int x, int y) {
    if (bs->s[XY_TO_BIDX(x, y)] != ID_BLANK) {
        return 1;
    }
    return 0;
}

int is_topleft(bsref *bs, int idx) {
    if (bs->s[idx] == ID_BLANK) {
        return 0;
    }
    int row = idx / 6;
    int col = idx % 6;
    if (row > 0) {
        if (bs->s[idx - 6] == bs->s[idx]) {
            return 0;
        }
    }
    if (col > 0) {
        if (bs->s[idx - 1] == bs->s[idx]) {
            return 0;
        }
    }
    return 1;
}

int get_id(bsref *bs, int x, int y) {
    return bs->s[XY_TO_BIDX(x, y)];
}


bool operator==(const bsref& l, const bsref& r) {
    return memcmp(l.s, r.s, 36) == 0;
}

int bsref_equal(bsref *a, bsref *b) {
    return memcmp(a->s, b->s, 36) == 0;
}

bool operator!=(const bsref& l, const bsref& r) {
    return memcmp(l.s, r.s, 36) != 0;
}

bool operator<(const bsref& l, const bsref& r) {
    return memcmp(l.s, r.s, 36);
}

bool operator>(const bsref& l, const bsref& r) {
    return memcmp(l.s, r.s, 36);
}

// if we did some zero-collapsing, we could probably get a more
//   compact hash by adding a check for is_topleft(bs, i).
//   otherwise omitting the extra ids seems silly
//   because all we're doing is throwing out data
// XXX: I just doubled our hash and called it the boardstate.
//   bstate is now 9 dwords, and id's can be up to 2 ** 8 (hurray!)
/*
void hash_board(bstate *bs, hash *h) {
    for (int i = 0; i < 5; i++) {
        h->b[i] = 0;
    }

    // do it row at a time
    for (int row = 0; row < 6; row++) {
        for (int i = 0; i < 6; i++) {
            int id = bs->s[XY_TO_BIDX(i, row)];
            h->b[row] |= id << (4 * (7 - i));
        }
    }
}
*/

int blocker_exists(node *c, int id) {
    for (int i = 0; i < NUM_BLOCKERS; i++) {
        if (c->blockers[i].id == id) {
            return 1;
        }
    }
    return 0;
}

// TODO: this currently allows multiple adds of same blocker
void add_blocker(node *c, int id, int dir) {
    if (id == c->id) {
        return;
    }
    if (blocker_exists(c, id)) {
        return;
    }
    //printf("add_blocker(%d, %d)\n", id, dir);
    for (int i = 0; i < NUM_BLOCKERS; i++) {
        if (c->blockers[i].id == ID_BLANK) {
            c->blockers[i].id = id;
            c->blockers[i].dir = dir;
            return;
        }
    }
}

void fill_node(node *c, int id) {
    c->id = id;
    c->init = 0;
    for (int i = 0; i < NUM_BLOCKERS; i++) {
        c->blockers[i].id = ID_BLANK;
        c->blockers[i].dir = NULL_DIR;
    }
}

// calculates the blockers and fills in ss.map
int calc_blockers(bsref *bs, depgraph *ss, int id) {
    if (id == ID_BLANK) {
        return -1;
    }
    if (!ss->map[id].init) {
        printf("no cb for !init\n");
        return -1;
    }

    node *c = &ss->map[id];
    c->init = 1;
    c->id = id;
    // look in either direction of move axis and add all seen to list
    int x, y;
    find_piece(bs, id, &x, &y);
    if (x == -1) {
        return -1;
    }
    if (is_horiz(bs, XY_TO_BIDX(x, y))) {
        for (int i = 0; i < 6; i++) {
            int other_id = bs->s[XY_TO_BIDX(i, y)];
            if (is_piece(bs, i, y) && id != other_id) {
                if (i < x) {
                    add_blocker(c, other_id, LEFT);
                } else {
                    add_blocker(c, other_id, RIGHT);
                }
            }
        }
    } else {
        for (int i = 0; i < 6; i++) {
            int other_id = bs->s[XY_TO_BIDX(x, i)];
            if (is_piece(bs, x, i) && id != other_id) {
                if (i < y) {
                    add_blocker(c, other_id, UP);
                } else {
                    add_blocker(c, other_id, DOWN);
                }
            }
        }
    }

    return 1;
}

// clones the contents of board bsa into board bsb
void bsref_clone(bsref *bsb, bsref *bsa) {
    for (int i = 0; i < 36; i++) {
        bsb->s[i] = bsa->s[i];
    }
}

void clear_bsref(bsref *h) {
    memset(h->s, 0x00, 36);
}

int is_null_hash(bsref *h) {
    return memcmp(h->s, null_bstate.s, 36);
}

int is_new_hash(bsref *bs) {
    return !sstack_contains(&seen, bs);
}

// enum all possible moves of the piece at bidx (ie x, y)
//   and if they aren't already in seen, explore them (by ret 1)
bool predict_next(bsref *bs, bsref *c_out, uint8_t bidx, int x, int y) {
    int id = bs->s[bidx];
    //printf("predict_next(%d @ %d, %d)\n", id, x, y);
    if (is_horiz(bs, bidx)) {
        if (x != 5) {
            for (int ix = 1; ix < 6; ix++) {
                if (bidx + ix >= 36) {
                    break;
                }
                if (bs->s[bidx + ix] == ID_BLANK) {
                    make_move(c_out, id, x, y, x + 1, y);
                    if (is_new_hash(c_out)) {
                        // this is our new move
                        sstack_push(&seen, c_out);
                        return 1;
                    }
                    make_move(c_out, id, x + 1, y, x , y);
                } else if (bs->s[bidx + ix] != id) {
                    break;
                }
            }
        }
        if (x != 0) {
            if (bs->s[bidx - 1] == ID_BLANK) {
                make_move(c_out, id, x, y, x - 1, y);
                if (is_new_hash(c_out)) {
                    sstack_push(&seen, c_out);
                    return 1;
                }
                make_move(c_out, id, x - 1, y, x, y);
            }
        }
    } else {
        if (y != 0) {
            if (bs->s[bidx - 6] == ID_BLANK) {
                make_move(c_out, id, x, y, x, y - 1);
                if (is_new_hash(c_out)) {
                    sstack_push(&seen, c_out);
                    return 1;
                }
                make_move(c_out, id, x, y - 1, x, y);
            }
        }
        if (y != 5) {
            for (int iy = 1; iy < 6; iy++) {
                if ((bidx + 6 * iy) >= 36) {
                    break;
                }
                if (bs->s[bidx + 6 * iy] == ID_BLANK) {
                    make_move(c_out, id, x, y, x, y + 1);
                    if (is_new_hash(c_out)) {
                        sstack_push(&seen, c_out);
                        return 1;
                    }
                    make_move(c_out, id, x, y + 1, x, y);
                } else if (bs->s[bidx + 6 * iy] != id) {
                    break;
                }
            }
        }
    }
    return 0;
}

// fill partial depgraph starting from curnode->id
void fill_depgraph(bsref *bs, depgraph *ss, node *curnode) {
    int curid = curnode->id;
    ss->map[curid].init = 1;
    calc_blockers(bs, ss, curid);

    // walk the current set of blockers and continue graph generation
    for (int i = 0; i < NUM_BLOCKERS; i++) {
        blocker *bl = &curnode->blockers[i];
        if (bl->id != ID_BLANK) {
            node *a = &ss->map[curnode->blockers[i].id];
            if (a->init == 0) {
                a->init = 1;
                a->id = curnode->blockers[i].id;
                calc_blockers(bs, ss, a->id);
            }
        }
    }
}

int on_depgraph(bsref *bs, depgraph *ss, node *curnode, node *newnode) {
    dstack ds;
    dstack_init(&ds);

    dstack dseen;
    dstack_init(&dseen);

    dstack_push(&ds, curnode->id);
    dstack_push(&dseen, curnode->id);
    int searchid = newnode->id;
    while (!dstack_empty(&ds)) {
        int id = dstack_pop(&ds);
        for (int i = 0; i < NUM_BLOCKERS; i++) {
            blocker *bl = &ss->map[id].blockers[i];
            if (bl->id == searchid) {
                return 1;
            }
            if (!dstack_contains(&dseen, bl->id)) {
                dstack_push(&ds, bl->id);
                dstack_push(&dseen, bl->id);
            }
        }
    }
    return 0;
}

// consider "free moves" of cur
//   and moves of cur's blockers
// returns 1 if we have a viable move,
//   and sets c_out with the updated board state
int apply_heuristics(bsref *bs, depgraph *ss, node *curnode, bsref *c_out, int *cid_out) {
    int id = curnode->id;
    int x, y;
    find_piece(bs, id, &x, &y);
    if (x == -1) {
        printf("error in apply_heuristics\n");
    }
    int bidx = XY_TO_BIDX(x, y);

    bsref_clone(c_out, bs);
    // consider free moves for this piece, then player piece, then all other pieces
    if (predict_next(bs, c_out, bidx, x, y)) {
        *cid_out = id;
        return 1;
    }

    if (HEURISTIC_CREEP) {
        for (int i = ID_P; i < ID_MAX; i++) {
            if (i == id) {
                // we already did ourself
                continue;
            }
            int ox, oy;
            find_piece(bs, i, &ox, &oy);
            int obidx = XY_TO_BIDX(ox, oy);
            if (predict_next(bs, c_out, obidx, ox, oy)) {
                *cid_out = id;
                return 1;
            }
        }
    }


    // consider blocker moves
    for (int i = 0; i < NUM_BLOCKERS; i++) {
        int b_id = curnode->blockers[i].id;
        if (b_id != ID_BLANK) {
            int b_x, b_y;
            find_piece(bs, b_id, &b_x, &b_y);
            if (predict_next(bs, c_out, XY_TO_BIDX(b_x, b_y), b_x, b_y)) {
                *cid_out = b_id;
                return 1;
            }
        }
    }

    // try again for all of the possible other pieces to move
    //   that are *NOT* on our depgraph?
    fill_depgraph(bs, ss, curnode);
    for (int i = ID_P; i < 36; i++) {
        node *newnode = &ss->map[i];
        fill_depgraph(bs, ss, newnode);
        if (newnode->id != id) {
            if (!on_depgraph(bs, ss, curnode, newnode)) {
                int o_id = newnode->id;
                int o_x, o_y;
                find_piece(bs, o_id, &o_x, &o_y);
                if (predict_next(bs, c_out, XY_TO_BIDX(o_x, o_y), o_x, o_y)) {
                    *cid_out = o_id;
                    return 1;
                }
            }
        }
    }

    return 0;
}

void ai_solve(bsref *init, solve_result *r_out) {
    sstack_init(&seen);

    r_out->solved = 0;
    r_out->moves = -1;
    // the bottom of board_history is the initial board state.
    // the top of board_history is the current board state
    sstack board_history;
    sstack_init(&board_history);
    sstack_push(&board_history, init);

    int steps = 0;
    // the id of the current piece to move
    int curid = ID_P;
    bsref curboard;
    memset(&curboard, 0x00, sizeof(curboard));
    bsref_clone(&curboard, init);
    while (steps < 0x20) {
        depgraph ss;
        node *curnode = &ss.map[curid];

        // is it solved right now?
        int px, py;
        find_piece(&curboard, ID_P, &px, &py);
        if (px == 4) {
            r_out->solved = 1;
            r_out->moves = steps;
            return;
        }

        // create depgraph with default node values
        //  (ie all pre-allocated)
        // we need to re-wipe the depgraph if steps != 1
        //       because we just moved a piece.
        for (int i = 0; i < 36; i++) {
            fill_node(&ss.map[i], i);
        }

        fill_depgraph(&curboard, &ss, curnode);

        // now that we have generated the "blocking dependency graph" in ss
        // we try to pick a reasonable move based on heuristics
        // once a move has been exhausted, it is no longer considered,
        //   and we fall through to subsequent weighted heuristics

        // XXX: can we update the depgraph faster than wipe+regen?

        bsref c;
        if (!apply_heuristics(&curboard, &ss, curnode, &c, &curid)) {
            // this is a dead end, so pop it off the stack
            if (sstack_empty(&board_history)) {
                //printf("error in is_solvable (board_history empty)\n");
                r_out->solved = 0;
                r_out->moves = steps;
                return;
            }

            bsref top;
            sstack_pop(&board_history, &top);
            steps++;
            //printf("dead end, board_history size is %d\n", sstack_size(&board_history));
        } else {
            sstack_push(&board_history, &c);
            bsref_clone(&curboard, &c);
            steps++;
        }
    }
    //printf("hit max step length -- give up\n");
    r_out->solved = 0;
    r_out->moves = steps;
    return;
}

void place_piece(bsref *out, int idx, int id) {
    int x = BIDX_TO_X(idx);
    int y = BIDX_TO_Y(idx);
    if (out->s[idx] == ID_BLANK) {
        if ((random() % 2) == 0) {
            if (x != 0) {
                if (out->s[idx - 1] == ID_BLANK) {
                    out->s[idx - 1] = id;
                    out->s[idx] = id;
                    return;
                }
            } else {
                if (out->s[idx + 1] == ID_BLANK) {
                    out->s[idx + 1] = id;
                    out->s[idx] = id;
                    return;
                }
            }

            if (y != 0) {
                if (out->s[idx - 6] == ID_BLANK) {
                    out->s[idx - 6] = id;
                    out->s[idx] = id;
                    return;
                }
            } else {
                if (out->s[idx + 6] == ID_BLANK) {
                    out->s[idx + 6] = id;
                    out->s[idx] = id;
                    return;
                }
            }
        } else {
            if (y != 0) {
                if (out->s[idx - 6] == ID_BLANK) {
                    out->s[idx - 6] = id;
                    out->s[idx] = id;
                    return;
                }
            } else {
                if (out->s[idx + 6] == ID_BLANK) {
                    out->s[idx + 6] = id;
                    out->s[idx] = id;
                    return;
                }
            }

            if (x != 0) {
                if (out->s[idx - 1] == ID_BLANK) {
                    out->s[idx - 1] = id;
                    out->s[idx] = id;
                    return;
                }
            } else {
                if (out->s[idx + 1] == ID_BLANK) {
                    out->s[idx + 1] = id;
                    out->s[idx] = id;
                    return;
                }
            }
        }
    }
}

// attempt to generate a random board - returns 1 on success
int generate_board(bsref *out) {
    clear_bsref(out);
    int num_pieces = 4 + random() % 10;

    int pidx = XY_TO_BIDX(0, random() % 6);
    out->s[pidx] = ID_P;
    out->s[pidx + 1] = ID_P;
    for (int i = ID_P + 1; i < num_pieces; i++) {
        int idx = random() % 36;
        place_piece(out, idx, i);
    }
    return 0;
}

int id_to_boardtype(bsref *b, int id) {
    switch (id) {
        case ID_BLANK:
            return 1;
        case ID_P:
            return 0;
        default:
            int x, y;
            find_piece(b, id, &x, &y);
            if (is_horiz(b, XY_TO_BIDX(x, y))) {
                return 2;
            } else {
                return 4;
            }
    }
}

void write_board(bsref *board, solve_result *sr, int file_id) {
    char path[1024];
    sprintf(&path[0], "../data/genboard%d", file_id);
    FILE *fp = fopen(&path[0], "w");
    fprintf(fp, "%d\n", sr->moves);
    for (int i = 0; i < 36; i++) {
        fprintf(fp, "%d", id_to_boardtype(board, board->s[i]));
        if ((i % 6) == 5) {
            fprintf(fp ,"\n");
        }
    }
    fclose(fp);
}

int main() {
    srandom(time(NULL));
    memset(&null_bstate, 0x00, sizeof(null_bstate));
    memset(&board_init, 0x00, sizeof(board_init));
    clear_bsref(&null_bstate);
    // initialize global bstate board_init with test board
    board_init.s[12] = 1;
    board_init.s[13] = 1;
    board_init.s[10] = 2;
    board_init.s[11] = 2;
    board_init.s[14] = 3;
    board_init.s[20] = 3;
    board_init.s[15] = 4;
    board_init.s[21] = 4;
    board_init.s[22] = 5;
    board_init.s[23] = 5;

    sstack gen_seen;
    sstack_init(&gen_seen);

    int out_id = 0;
    while (out_id < NUM_GEN) {
        generate_board(&board_init);
        solve_result sr;
        write_board(&board_init, &sr, out_id++);
        ai_solve(&board_init, &sr);
        if (sr.solved == 1 && sr.moves > MIN_MOVES) {
            if (!sstack_contains(&gen_seen, &board_init)) {
                print_board(&board_init);
                fprintf(stderr, "solve in %d\n", sr.moves);
                write_board(&board_init, &sr, out_id++);
                sstack_push(&gen_seen, &board_init);
            }
        } else {
            fprintf(stderr, "no solve\n");
        }
    }

    return 0;
}
