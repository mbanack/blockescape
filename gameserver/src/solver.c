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

using namespace std;

// run as "g++ -o solver solver.c && ./solver" from src/ dir
//
// TODO: hints
// TODO: 1x3 pieces at boardgen

// the minimum number of moves to solve the puzzle
#define MIN_MOVES 4
#define SHOW_MOVES 0
#define SHOW_DEPGRAPH 0
#define DEPGRAPH_DEPTH 3
#define SPIN_DIFFICULT 1
// TODO: obo check this
#define MAX_PIECES ID_MAX
#define MOVE_DIFF_RANGE 20

#define NUM_PIECES_RANGE 24

#define MAX(a, b) (((a) < (b)) ? (b) : (a))

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

int num_generated;

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
    int x, y, bidx;
    if (find_piece(bs, ID_P, &x, &y, &bidx)) {
        if (!is_horiz(bs, bidx)) {
            printf("{!!!} ID_P not horiz\n");
            exit(-1);
        }
    }
    for (int i = 0; i < 36; i++) {
        printf("%c ", id_to_hex(bs->s[i]));
        if ((i + 1) % 6 == 0) {
            printf("\n");
        }
    }
    printf("\n");
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
        print_board(bs);
        exit(11);
    }
    return 0;
}

// attempts to find a piece with given id, and returns its loc in x,y,bidx
int find_piece(bsref *bs, int id, int *x_out, int *y_out, int *bidx_out) {
    *x_out = -1;
    *y_out = -1;
    *bidx_out = -1;

    for (int i = 0; i < 36; i++) {
        if (bs->s[i] == id && is_topleft(bs, i)) {
            *bidx_out = i;
            *x_out = BIDX_TO_X(i);
            *y_out = BIDX_TO_Y(i);
            return 1;
        }
    }
    return 0;
}

int calc_width(bsref *bs, int id) {
    int x, y, bidx;
    if (find_piece(bs, id, &x, &y, &bidx)) {
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
    int x, y, bidx;
    if (find_piece(bs, id, &x, &y, &bidx)) {
        int h = 0;
        for (int bidx = XY_TO_BIDX(x, y); bidx < 36; bidx+=6) {
            if (bs->s[bidx] != id) {
                return h;
            }
            if (bidx >= 30) {
                return h + 1;
            }
            h++;
        }
    }
    return -1;
}

void make_move(bsref *bs, int id, int old_x, int old_y, int new_x, int new_y) {
    int bidx = XY_TO_BIDX(old_x, old_y);
    if (id == ID_BLANK) {
        printf("cannot make_move(ID_BLANK)\n");
        exit(12);
    }
    if (is_horiz(bs, bidx)) {
        int width = calc_width(bs, id);
        insert_piece(bs, ID_BLANK, width, 1, old_x, old_y);
        insert_piece(bs, id,       width, 1, new_x, new_y);
    } else {
        int height = calc_height(bs, id);
        insert_piece(bs, ID_BLANK, 1, height, old_x, old_y);
        insert_piece(bs, id,       1, height, new_x, new_y);
    }
}

void delete_piece(bsref *bs, int id) {
    int x, y, bidx;
    if (find_piece(bs, id, &x, &y, &bidx)) {
        if (is_horiz(bs, bidx)) {
            int width = calc_width(bs, id);
            insert_piece(bs, ID_BLANK, width, 1, x, y);
            for (int i = 0; i < 36; i++) {
                if (bs->s[i] > id) {
                    bs->s[i] = bs->s[i] - 1;
                }
            }
        } else {
            int height = calc_height(bs, id);
            insert_piece(bs, ID_BLANK, 1, height, x, y);
            for (int i = 0; i < 36; i++) {
                if (bs->s[i] > id) {
                    bs->s[i] = bs->s[i] - 1;
                }
            }
        }
    }
}

void make_move_dir(bsref *bs, int id, int dir) {
    int x, y, bidx;
    if (!find_piece(bs, id, &x, &y, &bidx)) {
        printf("cannot find piece %d\n", id);
        return;
    }

    if (id == ID_BLANK) {
        printf("cannot make_move_dir(ID_BLANK)\n");
        exit(12);
    }

    int width = calc_width(bs, id);
    int height = calc_height(bs, id);
    switch (dir) {
        case RIGHT:
            insert_piece(bs, ID_BLANK, width, 1, x, y);
            insert_piece(bs, id,       width, 1, x + 1, y);
            break;
        case LEFT:
            insert_piece(bs, ID_BLANK, width, 1, x, y);
            insert_piece(bs, id,       width, 1, x - 1, y);
            break;
        case DOWN:
            insert_piece(bs, ID_BLANK, 1, height, x, y);
            insert_piece(bs, id,       1, height, x, y + 1);
            break;
        case UP:
            insert_piece(bs, ID_BLANK, 1, height, x, y);
            insert_piece(bs, id,       1, height, x, y - 1);
            break;
        default:
            break;
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
    int x, y, bidx;
    find_piece(bs, id, &x, &y, &bidx);
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
    bsb->sr.solved = bsa->sr.solved;
    bsb->sr.num_pieces = bsa->sr.num_pieces;
    bsb->sr.moves = bsa->sr.moves;
}

void clear_bsref(bsref *h) {
    memset(h->s, 0x00, 36);
    h->sr.solved = 0;
    h->sr.moves = 0;
    h->sr.num_pieces = 0;
}

int is_null_hash(bsref *h) {
    return memcmp(h->s, null_bstate.s, 36);
}

int is_new_hash(bsref *bs) {
    return !sstack_contains(&seen, bs);
}

// assumes move is valid for piece
int move_unseen(bsref *bs, int id, int x, int y, int bidx, int dir) {
    bsref work;
    bsref_clone(&work, bs);
    switch (dir) {
        case RIGHT:
            break;
        case LEFT:
            break;
        case DOWN:
            break;
        case UP:
            break;
        default:
            return 0;
            break;
    }
}

// if a move is found, returns 1 and sets dir_out to the direction to be moved
// else returns 0
int piece_moves(bsref *bs, int id, int *dir_out) {
    int x, y, bidx;
    if (id == ID_BLANK || !find_piece(bs, id, &x, &y, &bidx)) {
        return 0;
    }

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
                    if (move_unseen(bs, id, x, y, bidx, RIGHT)) {
                        *dir_out = RIGHT;
                        return 1;
                    }
                } else if (bs->s[bidx + ix] != id) {
                    break;
                }
            }
        }
        if (x != 0) {
            if (bs->s[bidx - 1] == ID_BLANK) {
                if (move_unseen(bs, id, x, y, bidx, LEFT)) {
                    *dir_out = LEFT;
                    return 1;
                }
            }
        }
    } else {
        if (y != 0) {
            if (bs->s[bidx - 6] == ID_BLANK) {
                if (move_unseen(bs, id, x, y, bidx, UP)) {
                    *dir_out = UP;
                    return 1;
                }
            }
        }
        if ((y + calc_height(bs, bs->s[bidx]) - 1) != 5) {
            for (int iy = 1; iy < 6; iy++) {
                if ((bidx + 6 * iy) >= 36) {
                    break;
                }
                if (bs->s[bidx + 6 * iy] == ID_BLANK) {
                    if (move_unseen(bs, id, x, y, bidx, DOWN)) {
                        *dir_out = DOWN;
                        return 1;
                    }
                } else if (bs->s[bidx + 6 * iy] != id) {
                    break;
                }
            }
        }
    }
    return 0;
}

// enum all possible moves of the piece at bidx (ie x, y)
//   and if they aren't already in seen, explore them (by ret 1)
int predict_next(bsref *bs, bsref *c_out, uint8_t bidx, int x, int y) {
    int id = bs->s[bidx];
    if (id == ID_BLANK) {
        return 0;
    }
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
                        return RIGHT;
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
                    return LEFT;
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
                if (is_new_hash(c_out)) {
                    if (SHOW_MOVES) {
                        printf("MOVE %d UP\n", id);
                    }
                    sstack_push(&seen, c_out);
                    return UP;
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
                        return DOWN;
                    }
                    make_move(c_out, id, x, y + 1, x, y);
                } else if (bs->s[bidx + 6 * iy] != id) {
                    break;
                }
            }
        }
    }
    return NULL_DIR;
}

void clear_depgraph(depgraph *ss) {
    for (int id = 0; id <= ID_MAX; id++) {
        node *a = &ss->map[id];
        a->id = id;
        a->init = 0;
        memset(&a->blockers, 0x00, sizeof(blocker) * NUM_BLOCKERS);
    }
}

// fill entire depgraph (automatically clears/inits the depgraph)
void fill_full_depgraph(bsref *bs, depgraph *ss) {
    clear_depgraph(ss);
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

int solved(bsref *bs) {
    int px, py, pbidx;
    if (find_piece(bs, ID_P, &px, &py, &pbidx)) {
        if (px == 4) {
            return 1;
        }
    }
    return 0;
}

void ai_solve(bsref *init, solve_result *r_out, int max_steps) {
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
    bsref curboard;
    memset(&curboard, 0x00, sizeof(curboard));
    bsref_clone(&curboard, init);
    while (steps < max_steps) {
        if (solved(&curboard)) {
            r_out->solved = 1;
            r_out->moves = steps;
            return;
        }

        int moved_id = ID_BLANK;
        int dir = NULL_DIR;
        heuristics(&curboard, &dir, &moved_id);
        if (dir == NULL_DIR) {
            // this is a dead end, so pop it off the stack
            if (sstack_empty(&board_history)) {
                r_out->solved = 0;
                r_out->moves = steps;
                return;
            }

            bsref top;
            sstack_pop(&board_history, &top);
            steps++;
            r_out->solved = 0;
            r_out->moves = steps;
            return;
        } else {
            make_move_dir(&curboard, moved_id, dir);
            sstack_push(&board_history, &curboard);
            steps++;
        }
    }
    r_out->solved = 0;
    r_out->moves = steps;
    return;
}

int place_piece(bsref *out, int idx, int id, int pidx) {
    int x = BIDX_TO_X(idx);
    int y = BIDX_TO_Y(idx);
    if (out->s[idx] == ID_BLANK) {
        if ((idx / 6) != (pidx / 6) && (random() % 2) == 0) {
            // horizontal
            if (x != 0) {
                if (out->s[idx - 1] == ID_BLANK) {
                    out->s[idx - 1] = id;
                    out->s[idx] = id;
                    return 1;
                }
            } else {
                if (out->s[idx + 1] == ID_BLANK) {
                    out->s[idx + 1] = id;
                    out->s[idx] = id;
                    return 1;
                }
            }

            if (y != 0) {
                if (out->s[idx - 6] == ID_BLANK) {
                    out->s[idx - 6] = id;
                    out->s[idx] = id;
                    return 1;
                }
            } else {
                if (out->s[idx + 6] == ID_BLANK) {
                    out->s[idx + 6] = id;
                    out->s[idx] = id;
                    return 1;
                }
            }
        } else {
            // vertical
            if (y != 0) {
                if (out->s[idx - 6] == ID_BLANK) {
                    out->s[idx - 6] = id;
                    out->s[idx] = id;
                    return 1;
                }
            } else {
                if (out->s[idx + 6] == ID_BLANK) {
                    out->s[idx + 6] = id;
                    out->s[idx] = id;
                    return 1;
                }
            }

            if (x != 0) {
                if (out->s[idx - 1] == ID_BLANK) {
                    out->s[idx - 1] = id;
                    out->s[idx] = id;
                    return 1;
                }
            } else {
                if (out->s[idx + 1] == ID_BLANK) {
                    out->s[idx + 1] = id;
                    out->s[idx] = id;
                    return 1;
                }
            }
        }
    }
    return 0;
}

// next avail id
int free_id(bsref *bs) {
    for (int i = ID_P; i <= ID_MAX; i++) {
        int x, y, bidx;
        if (!find_piece(bs, i, &x, &y, &bidx)) {
            return i;
        }
    }
    printf("{!!!} free_id() overflow\n");
    return ID_NUM;
}

void add_board(bsref *bs, sstack *gen_seen) {
    if (!sstack_contains(gen_seen, bs)) {
        printf("add_board(%d) with free_id %d\n", bs->sr.moves, free_id(bs));
        print_board(bs);
        bs->sr.num_pieces = free_id(bs);
        sstack_push(gen_seen, bs);
        num_generated++;
    }
}

// attempt to generate a random board of at least given # moves
// returns 1 on success
int generate_board(bsref *out, solve_result *sr, sstack *gen_seen, int moves) {
    printf("gen_board(... , %d) (have %d) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", moves, num_generated);
    clear_bsref(out);
    int pidx = XY_TO_BIDX(0, random() % 6);
    out->s[pidx] = ID_P;
    out->s[pidx + 1] = ID_P;
    int found = 0;
    int last_id = 0;

    for (int k = 0; k < 120; k++) {
        for (int i = 0; i < MAX_PIECES; i++) {
            if (free_id(out) == ID_MAX) {
                break;
            }
            int idx = random() % 36;
            for (int ii = 0; ii < 10; ii++) {
                last_id = free_id(out);
                if (place_piece(out, idx, last_id, pidx)) {
                    break;
                }
            }
            ai_solve(out, sr, moves + MOVE_DIFF_RANGE);
            if (sr->solved == 1) {
                if (sr->moves > moves) {
                    add_board(out, gen_seen);
                    return 1;
                }
            }
        }

        // delete some random pieces
        int num_del = 1 + (random() % 6);
        for (int dx = 0; dx < num_del; dx++) {
            int id = ID_P + 1 + (random() % (free_id(out) - 1));
            delete_piece(out, id);
        }
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
            int x, y, bidx;
            find_piece(b, id, &x, &y, &bidx);
            if (is_horiz(b, bidx)) {
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

// if there is a viable move, it is returned in dir_out and id_out
//   we probably dont care which block is "current" -- move the best one
// after generating the "blocking dependency graph" in ss
//   we try to pick a reasonable move based on heuristics
//   once a move has been exhausted, it is no longer considered,
//   and we fall through to subsequent weighted heuristics
void heuristics(bsref *bs, int *dir_out, int *id_out) {
    bsref work;
    bsref_clone(&work, bs);
    depgraph ss;
    clear_depgraph(&ss);
    fill_full_depgraph(bs, &ss);
    *dir_out = NULL_DIR;
    *id_out = ID_BLANK;

    // consider the depgraph of ID_P
    for (int bid = 0; bid < NUM_BLOCKERS; bid++) {
        blocker *b = &ss.map[ID_P].blockers[bid];
        if (b->id == ID_BLANK) {
            break;
        }
        if (piece_moves(bs, b->id, dir_out)) {
            *id_out = b->id;
            return;
        }
    }

    // consider free moves for the player piece
    if (piece_moves(bs, ID_P, dir_out)) {
        *id_out = ID_P;
        return;
    }

    // consider free moves for all other pieces
    //   that are *NOT* on ID_P's depgraph (else we already did them)
    node *p_node = &ss.map[ID_P];
    for (int id = ID_P + 1; id <= ID_MAX; id++) {
        node *newnode = &ss.map[id];
        if (newnode->id == ID_BLANK) {
            break;
        }
        if (!on_depgraph(bs, &ss, p_node, newnode)) {
            int o_id = newnode->id;
            int o_x, o_y, o_bidx;
            if (find_piece(bs, o_id, &o_x, &o_y, &o_bidx)) {
                if (piece_moves(bs, o_id, dir_out)) {
                    *id_out = o_id;
                    return;
                }
            }
        }
    }
}

void print_moves(bsref *b_init) {
    bsref work;
    bsref_clone(&work, b_init);
    int num_moves = 0;
    while (!solved(&work) && num_moves < 1024) {
        int moved_id = ID_BLANK;
        int dir = NULL_DIR;
        heuristics(&work, &dir, &moved_id);
        if (dir == NULL_DIR) {
            return;
        } else {
            printf("%d %s\n", moved_id, DIRNAMES[dir]);
            make_move_dir(&work, moved_id, dir);
            num_moves++;
        }
    }
}

int get_hint(bsref *bs, int *moved_id, int *dir) {
    bsref work;
    bsref_clone(&work, bs);
    *moved_id = ID_BLANK;
    *dir = NULL_DIR;
    heuristics(&work, dir, moved_id);
    if (*dir == NULL_DIR) {
        return 0;
    } else {
        return 1;
    }
}

int main(int argc, char **argv) {
    time_t sd = time(NULL) % 1024;
    //sd = 887;
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

    // arg handling
    int num_gen = 1;
    int max_steps = 0x30;
    if (argc > 1) {
        num_gen = atoi(argv[1]);
        printf("num_gen is %d\n", num_gen);
    }
    if (argc > 2) {
        max_steps = atoi(argv[2]);
    }

    int out_id = 0;
    num_generated = 0;
    while (num_generated < num_gen) {
        // the last param to gen_board is the desired num moves
        if (generate_board(&board_init, &board_init.sr, &gen_seen, 4 + (out_id + 3) / 3)) {
            printf("{puzzle %d %d}\n", out_id, board_init.sr.moves);
            print_board(&board_init);
            if (SHOW_MOVES) {
                print_moves(&board_init);
            }
            printf("{/puzzle %d %d}\n", out_id, board_init.sr.moves);
            add_board(&board_init, &gen_seen);
            write_board(&board_init, &board_init.sr, out_id);
            sstack_push(&gen_seen, &board_init);
            out_id++;
            num_generated++;
        }
    }

    // print out all the puzzles we found while hill-climbing for difficult ones
    for (int i = 0; i < gen_seen.idx; i++) {
        bsref *bs = &gen_seen.arr[i];
        printf("[%d %d %d] ", bs->sr.moves, bs->sr.num_pieces, bs->sr.solved);
        print_boardhash(bs);
        // TODO: write them out here (in some order by difficulty?)
    }

    printf("random seed is %d\n", sd);
    return 0;
}
