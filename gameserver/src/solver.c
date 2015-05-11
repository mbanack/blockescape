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
//
// TODO: hints
// TODO: 1x3 pieces at boardgen

#define NUM_GEN 1
// the minimum number of moves to solve the puzzle
#define MIN_MOVES 1
#define SHOW_MOVES 1
#define DEPGRAPH_DEPTH 3
#define MAX_STEPS 0x3

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

const char *DIRNAMES[] = {"LEFT", "RIGHT", "UP", "DOWN"};

// convert board idx in 0 .. 35 to x, y in 0 .. 5
#define BIDX_TO_X(bidx) ((bidx) % 6)
#define BIDX_TO_Y(bidx) ((bidx) / 6)
// convert x,y to board idx
#define XY_TO_BIDX(x, y) ((6 * (y) + (x)))

bsref null_bstate;
bsref board_init;
sstack seen;

// should we consider free moves of other pieces such as the player piece
const int HEURISTIC_CREEP = 1;

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

void sstack_print(sstack *s) {
    for (int i = 0; i < s->idx; i++) {
        printf("[%3d] ", i);
        bsref_print(&s->arr[i]);
        printf("\n");
    }
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
            for (int i = 0; i < NUM_BLOCKERS; i++) {
                if (n->blockers[i].id != ID_BLANK) {
                    if (i == 0) {
                        printf("node %d\n", n->id);
                    }
                    printf("  %d %s depth %d\n", n->blockers[i].id,
                                                 DIRNAMES[n->blockers[i].dir],
                                                 n->blockers[i].depth);
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
    if (bs->s[idx] == ID_BLANK) {
        return 0;
    }
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
    if (col == 5 && (bs->s[idx + 1] == bs->s[idx])) {
        printf("error: piece %d over edge\n", idx);
        exit(11);
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
    printf("calc_height couldnt find piece id %d\n", id);
    print_board(bs);
    exit(16);
    return -1;
}

void make_move(bsref *bs, int id, int old_x, int old_y, int new_x, int new_y) {
    int bidx = XY_TO_BIDX(old_x, old_y);
    printf("make_move(%d, %d, %d, %d, %d)\n", id, old_x, old_y, new_x, new_y);
    if (id == ID_BLANK) {
        printf("cannot make_move(ID_BLANK)\n");
        exit(12);
    }
    if (is_horiz(bs, bidx)) {
        int width = calc_width(bs, id);
        insert_piece(bs, ID_BLANK, width, 1, old_x, old_y);
        insert_piece(bs, id,       width, 1, new_x, new_y);
    } else {
        printf("is_vert\n");
        int height = calc_height(bs, id);
        printf("height is %d\n", height);
        insert_piece(bs, ID_BLANK, 1, height, old_x, old_y);
        insert_piece(bs, id,       1, height, new_x, new_y);
    }
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

void bsref_print(bsref *a) {
    for (int i = 0; i < 36; i++) {
        printf("%c", id_to_hex(a->s[i]));
    }
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

void add_blocker(node *c, int id, int dir, int depth) {
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
            c->blockers[i].depth = depth;
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
int calc_blockers(bsref *bs, depgraph *ss, int id, int depth) {
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
                    add_blocker(c, other_id, LEFT, depth);
                } else {
                    add_blocker(c, other_id, RIGHT, depth);
                }
            }
        }
    } else {
        for (int i = 0; i < 6; i++) {
            int other_id = bs->s[XY_TO_BIDX(x, i)];
            if (is_piece(bs, x, i) && id != other_id) {
                if (i < y) {
                    add_blocker(c, other_id, UP, depth);
                } else {
                    add_blocker(c, other_id, DOWN, depth);
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
    if (id == ID_BLANK) {
        return 0;
    }
    printf("predict_next(%d @ %d, %d)\n", id, x, y);
    if (is_horiz(bs, bidx)) {
        if ((x + calc_width(bs, bidx) - 1) != 5) {
            for (int ix = 1; ix < 6; ix++) {
                if (bidx + ix >= 36) {
                    break;
                }
                if (((bidx + ix) % 6) < (bidx % 6)) {
                    break;
                }
                if (bs->s[bidx + ix] == ID_BLANK) {
                    make_move(c_out, id, x, y, x + 1, y);
                    if (is_new_hash(c_out)) {
                        // this is our new move
                        if (SHOW_MOVES) {
                            printf("MOVE %d RIGHT\n", id);
                        }
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
                    if (SHOW_MOVES) {
                        printf("MOVE %d LEFT\n", id);
                    }
                    sstack_push(&seen, c_out);
                    return 1;
                }
                make_move(c_out, id, x - 1, y, x, y);
            }
        }
    } else {
        if (y != 0) {
            printf("id is %d y not 0 vert\n", id);
            if (bs->s[bidx - 6] == ID_BLANK) {
                printf("!! id is %d y not 0 vert\n", id);
                make_move(c_out, id, x, y, x, y - 1);
                print_board(c_out);
                if (is_new_hash(c_out)) {
                    if (SHOW_MOVES) {
                        printf("MOVE %d UP\n", id);
                    }
                    sstack_push(&seen, c_out);
                    return 1;
                } else {
                    printf("old news...\n");
                }
                make_move(c_out, id, x, y - 1, x, y);
            }
        }
        if ((y + calc_height(bs, bs->s[bidx]) - 1) != 5) {
            for (int iy = 1; iy < 6; iy++) {
                if ((bidx + 6 * iy) >= 36) {
                    break;
                }
                if (bs->s[bidx + 6 * iy] == ID_BLANK) {
                    make_move(c_out, id, x, y, x, y + 1);
                    if (is_new_hash(c_out)) {
                        if (SHOW_MOVES) {
                            printf("MOVE %d DOWN\n", id);
                        }
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

void clear_depgraph(depgraph *ss) {
    for (int id = 0; id <= ID_MAX; id++) {
        node *a = &ss->map[id];
        a->id = id;
        a->init = 0;
        memset(&a->blockers, 0x00, sizeof(blocker) * NUM_BLOCKERS);
    }
}

// fill entire depgraph
void fill_full_depgraph(bsref *bs, depgraph *ss) {
    // calculate the immediate blockers (depth 0)
    for (int id = ID_P; id <= ID_MAX; id++) {
        node *a = &ss->map[id];
        a->init = 1;
        calc_blockers(bs, ss, id, 0);
    }

    // propagate dependencies
    for (int n = 0; n < DEPGRAPH_DEPTH; n++) {
        for (int id = ID_P; id <= ID_MAX; id++) {
            node *blocked = &ss->map[id];
            for (int bx = 0; bx < NUM_BLOCKERS; bx++) {
                blocker *b = &blocked->blockers[bx];
                if (b->id != ID_BLANK) {
                    // add to blocked all of b's blockers with depth + 1
                    for (int bxx = 0; bxx < NUM_BLOCKERS; bxx++) {
                        blocker *sub = &ss->map[b->id].blockers[bxx];
                        if (sub->id != ID_BLANK) {
                            if (!blocker_exists(blocked, sub->id)) {
                                // keep the same block direction
                                //   ie ID_P is blocked RIGHT by id8, sub id2, sub id4
                                add_blocker(blocked, sub->id, b->dir, b->depth + 1);
                            }
                        }
                    }
                }
            }
        }
    }
}

// fill partial depgraph starting from curnode->id
void fill_depgraph(bsref *bs, depgraph *ss, node *curnode) {
    int curid = curnode->id;
    ss->map[curid].init = 1;
    calc_blockers(bs, ss, curid, 0);

    // walk the current set of blockers and continue graph generation
    for (int i = 0; i < NUM_BLOCKERS; i++) {
        blocker *bl = &curnode->blockers[i];
        if (bl->id != ID_BLANK) {
            node *a = &ss->map[curnode->blockers[i].id];
            if (a->init == 0) {
                a->init = 1;
                a->id = curnode->blockers[i].id;
                calc_blockers(bs, ss, a->id, 0);
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
    printf("ah(%d)\n", id);
    int x, y;
    find_piece(bs, id, &x, &y);
    if (x == -1) {
        printf("error in apply_heuristics\n");
    }
    int bidx = XY_TO_BIDX(x, y);
    bsref_clone(c_out, bs);

    // consider the depgraph of ID_P
    for (int bid = 0; bid < NUM_BLOCKERS; bid++) {
        blocker *b = &ss->map[ID_P].blockers[bid];
        if (b->id == ID_BLANK) {
            break;
        }
        int b_x, b_y;
        find_piece(bs, b->id, &b_x, &b_y);
        int b_bidx = XY_TO_BIDX(b_x, b_y);
        if (predict_next(bs, c_out, b_bidx, b_x, b_y)) {
            printf("{HA %d}\n", b->id);
            *cid_out = b->id;
            return 1;
        }
    }

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
            if (ox == -1) {
                continue;
            }
            int obidx = XY_TO_BIDX(ox, oy);
            printf("(%d, %d) => %d\n", ox, oy, obidx);
            printf(" ah(%d)->%d\n", id, bs->s[obidx]);
            if (predict_next(bs, c_out, obidx, ox, oy)) {
                *cid_out = i;
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
            if (b_x == -1) {
                continue;
            }
            if (predict_next(bs, c_out, XY_TO_BIDX(b_x, b_y), b_x, b_y)) {
                *cid_out = b_id;
                return 1;
            }
        }
    }

    // try again for all of the possible other pieces to move
    //   that are *NOT* on our depgraph?
    fill_full_depgraph(bs, ss);
    for (int i = ID_P; i < 36; i++) {
        node *newnode = &ss->map[i];
        if (newnode->id != id) {
            if (!on_depgraph(bs, ss, curnode, newnode)) {
                int o_id = newnode->id;
                int o_x, o_y;
                find_piece(bs, o_id, &o_x, &o_y);
                if (o_x == -1) {
                    continue;
                }
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
    sstack_push(&seen, init);

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
    while (steps < MAX_STEPS) {
        depgraph ss;
        clear_depgraph(&ss);
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

        //fill_depgraph(&curboard, &ss, curnode);
        fill_full_depgraph(&curboard, &ss);
        printf("[step %d]=================================\n", steps);
        print_board(&curboard);
        print_depgraph(&ss);
        printf("\n");

        // now that we have generated the "blocking dependency graph" in ss
        // we try to pick a reasonable move based on heuristics
        // once a move has been exhausted, it is no longer considered,
        //   and we fall through to subsequent weighted heuristics

        // XXX: can we update the depgraph faster than wipe+regen?

        bsref c;
        if (!apply_heuristics(&curboard, &ss, curnode, &c, &curid)) {
            // this is a dead end, so pop it off the stack
            if (sstack_empty(&board_history)) {
                printf("error in is_solvable (board_history empty)\n");
                r_out->solved = 0;
                r_out->moves = steps;
                return;
            }

            bsref top;
            sstack_pop(&board_history, &top);
            steps++;
            printf("dead end, board_history size is %d\n", sstack_size(&board_history));
            //sstack_print(&board_history);
            exit(1);
        } else {
            sstack_push(&board_history, &c);
            bsref_clone(&curboard, &c);
            steps++;
        }
    }
    printf("hit max step length -- give up\n");
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
    time_t sd = time(NULL) % 1024;
    sd = 191;
    printf("random seed is %d\n", sd);
    srandom(sd);

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

    printf("random seed is %d\n", sd);
    return 0;
}
