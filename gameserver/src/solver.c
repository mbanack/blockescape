#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stack>
#include <set>

#include "solver.h"

using namespace std;

// board is 6x6

// TODO: helper sugars for "print current board state"
//         and "print current tree/table state"
//
// space reqs at end:
//    board_history will cap out at 128 or something arbitrary (max solve len)
//    seen and unproductive (which could easily be merged)
//      could grow rather large.
//      we may be able to analyze seen "hits" and try to throw out
//        the ones that heuristically never hit

#define ID_BLANK 0x00
#define ID_P 0x01

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

bstate null_bstate;
bstate board_init;
set<bstate> seen;
set<bstate> unproductive;
// the bottom of board_history is the initial board state.
// the top of board_history is the current board state
stack<bstate> board_history;

uint8_t id_to_hex(int id) {
    if (id < 10) {
        return 48 + id; // decimal conv.
    } else {
        return 55 + id; // hex conv.
    }
}

void print_board(bstate bs) {
    printf("===\n");
    for (int i = 0; i < 36; i++) {
        printf("%c", id_to_hex(bs[i]));
        if ((i + 1) % 6 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void insert_piece(bstate bs, int id, int width, int height, int new_x, int new_y) {
    int bidx = XY_TO_BIDX(new_x, new_y);
    if (width != 1) {
        for (int i = 0; i < width; i++) {
            bs[bidx + i] = id;
        }
    } else {
        for (int i = 0; i < height; i++) {
            bs[bidx + (6 * i)] = id;
        }
    }
}

int is_horiz(bstate bs, int idx) {
    int col = idx % 6;
    if (col != 0) {
        if (bs[idx - 1] == bs[idx]) {
            return 1;
        }
    }
    if (col != 5) {
        if (bs[idx + 1] == bs[idx]) {
            return 1;
        }
    }
    return 0;
}

// attempts to find a piece with given id, and returns its loc in x,y
void find_piece(bstate bs, int id, int *x, int *y) {
    *x = -1;
    *y = -1;

    for (int i = 0; i < 36; i++) {
        if (bs[i] == id && is_topleft(bs, i)) {
            *x = BIDX_TO_X(i);
            *y = BIDX_TO_Y(i);
            return;
        }
    }
}

int calc_width(bstate bs, int id) {
    int x, y;
    find_piece(bs, id, &x, &y);
    if (x != -1) {
        int w = 1;
        for (int bidx = XY_TO_BIDX(x, y); bidx < 36; bidx++) {
            if (bs[bidx] != id) {
                return w;
            }
            w++;
        }
    }
    return -1;
}

int calc_height(bstate bs, int id) {
    int x, y;
    find_piece(bs, id, &x, &y);
    if (x != -1) {
        int h = 1;
        for (int bidx = XY_TO_BIDX(x, y); bidx < 36; bidx+=6) {
            if (bs[bidx] != id) {
                return h;
            }
            h++;
        }
    }
    return -1;
}

void make_move(bstate bs, int id, int old_x, int old_y, int new_x, int new_y) {
    printf("make_move(%d => %d, %d)\n", id, new_x, new_y);
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
}

// clones the contents of board bsa into board bsb
void clone_bstate(bstate bsb, bstate bsa) {
    strncpy((char *)bsb, (char *)bsa, 36);
}

int is_piece(bstate bs, int x, int y) {
    if (bs[XY_TO_BIDX(x, y)] != ID_BLANK) {
        return 1;
    }
    return 0;
}

int is_topleft(bstate bs, int idx) {
    if (bs[idx] == ID_BLANK) {
        return 0;
    }
    int row = idx / 6;
    int col = idx % 6;
    if (row > 0) {
        if (bs[idx - 6] == bs[idx]) {
            return 0;
        }
    }
    if (col > 0) {
        if (bs[idx - 1] == bs[idx]) {
            return 0;
        }
    }
    return 1;
}

int get_id(bstate bs, int x, int y) {
    return bs[XY_TO_BIDX(x, y)];
}


bool operator==(const bsref& l, const bsref& r) {
    return memcmp(l.bs, r.bs, 36) == 0;
}

bool operator!=(const bsref& l, const bsref& r) {
    return memcmp(l.bs, r.bs, 36) != 0;
}

bool operator<(const bsref& l, const bsref& r) {
    return memcmp(l.bs, r.bs, 36);
}

bool operator>(const bsref& l, const bsref& r) {
    return memcmp(l.bs, r.bs, 36);
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
            int id = bs[XY_TO_BIDX(i, row)];
            h->b[row] |= id << (4 * (7 - i));
        }
    }
}
*/

void add_blocker(node *c, int id, int dir) {
    printf("add_blocker( %d, %d)\n", id, dir);
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
int calc_blockers(bstate bs, solvestate *ss, int id) {
    if (id == ID_BLANK) {
        return -1;
    }
    printf("calc_blockers(%d)\n", id);
    if (!ss->map[id].init) {
        printf("no cb for !init\n");
        return -1;
    }

    node *c = &ss->map[id];
    c->init = 1;
    // look in either direction of move axis and add all seen to list
    int x, y;
    find_piece(bs, id, &x, &y);
    if (x == -1) {
        return -1;
    }
    if (is_horiz(bs, id)) {
        for (int i = 0; i < 6; i++) {
            int other_id = bs[XY_TO_BIDX(i, y)];
            printf("cb i=%d y=%d\n", i, y);
            printf("  %d %d\n", is_piece(bs, i, y), id != other_id);
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
            int other_id = bs[XY_TO_BIDX(x, i)];
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

/*
// pop all hashes until we see the given hash
//   adding all popped hashes to an "unproductive" list
// TODO: also needs the bstate as a parameter...
//       needs to "forward-wind from the beginning"
//       to go back to the previous board state
//  OR: apply "anti-moves"
//
//
// from a board hash and parsing the board_init,
//   we can glean the size of the pieces, and overlay
//   them on the hash
// wait...
// if we're not collapsing zeroes, it doesnt matter that
//   we don't only store the topleft.
void rewind(hash h) {
    hash top = board_history.top();
    while (top != h) {
        unproductive.insert(top);
        board_history.pop();
        top = board_history.top();
    }
}
*/

void clear_bstate(bstate h) {
    memset(h, 0x00, 36);
}

int is_null_hash(bstate h) {
    return memcmp(h, null_bstate, 36);
}

void predict_next(uint8_t id, uint8_t dir, bstate next) {
    clear_bstate(next);

    // enum all possible moves of that piece (given cur)
    //   and if they aren't already in seen, explore them
}

int consider_blockers(bstate bs, solvestate *ss, int *curid) {
    node *cur = &ss->map[*curid];
    for (int i = 0; i < NUM_BLOCKERS; i++) {
        if (cur->blockers[i].id != ID_BLANK) {
            bstate predict;
            predict_next(cur->blockers[i].id, cur->blockers[i].dir,
                         predict);
            if (seen.count(predict) == 0) {
                // we haven't seen it, so try it
                *curid = cur->blockers[i].id;
                return 1;
            }
        }
    }
    return 0;
}

int is_new_hash(bstate bs) {
    return seen.count(bs) == 0;
}

// consider "free moves" of cur
//   and moves of cur's blockers
// if we have a viable move
//   push the new hash onto board_history
//   update curboard
int apply_heuristics(bstate bs, solvestate *ss, node *curnode) {
    int id = curnode->id;
    int x, y;
    int bidx = XY_TO_BIDX(x, y);
    find_piece(bs, id, &x, &y);
    if (x == -1) {
        printf("error in apply_heuristics\n");
    }

    // consider free moves
    if (is_horiz(bs, bidx)) {

    } else {

    }

    // if no free moves, consider blockers
    if (!consider_blockers(bs, ss, &id)) {
        // ...
    }

    // TODO: if we have a nice way to shortcut multiple moves, need to update steps in scope-above
    //make_move(curboard, curid, old_x, old_y, new_x, new_y);
    //
    //board_history.push(h)
    //seen.insert(h);

    return false;
}


// iterate on a pre-loaded board_history
int is_solvable(bstate init) {
    int steps = 0;
    // the id of the current piece to move
    int curid;
    bstate curboard;
    clone_bstate(init, curboard);
    while (steps < 0xFFFF) {
        print_board(curboard);
        solvestate ss;
        node *curnode = &ss.map[curid];

        // create solvestate with default node values
        //  (ie all pre-allocated)
        // we need to re-wipe the node solvestate if steps != 1
        //       because we just moved a piece.
        for (int i = 0; i < 36; i++) {
            fill_node(&ss.map[i], i);
        }
        if (steps == 0) {
            curid = ss.map[ID_P].id;
            // starting from P (id 1)
            ss.map[ID_P].init = 1;
            calc_blockers(curboard, &ss, ID_P);
        }

        // is it solved right now?
        int px, py;
        find_piece(curboard, ID_P, &px, &py);
        if (px == 4) {
            return 1;
        }

        // walk the current set of blockers and continue graph generation
        for (int i = 0; i < NUM_BLOCKERS; i++) {
            node *a = &ss.map[curnode->blockers[i].id];
            if (a->init == 0) {
                a->init = 1;
                calc_blockers(curboard, &ss, a->id);
            }
        }

        // now that we have generated the "blocking dependency graph" in ss
        // we try to pick a reasonable move based on heuristics
        // once a move has been exhausted, it is no longer considered,
        //   and we fall through to subsequent weighted heuristics

        // XXX: can we update the depgraph faster than wipe+regen?

        if (!apply_heuristics(curboard, &ss, curnode)) {
            // this is a dead end, so pop it off the stack
            unproductive.insert(board_history.top());
            board_history.pop();
        } else {
            steps++;
        }
    }
    printf("0xFFFF\n");
    return 0;
}

int main() {
    clear_bstate(&null_bstate);
    // initialize global bstate board_init with test board
    board_init[12] = 1;
    board_init[13] = 1;
    board_init[10] = 2;
    board_init[11] = 2;
    board_init[14] = 3;
    board_init[20] = 3;
    board_init[18] = 4;
    board_init[19] = 4;
    board_init[22] = 5;
    board_init[23] = 5;

    // preload bottom of board_history
    hash init_hash;
    hash_board(&board_init, &init_hash);
    board_history.push(init_hash);

    print_board();

    if (is_solvable(&board_init)) {
        printf("solve\n");
    } else {
        printf("no solve\n");
    }
    return 0;
}
