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

bsref null_bstate;
bsref board_init;
set<bsref> seen;
set<bsref> unproductive;

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
    for (std::set<bsref>::iterator it = seen.begin();
         it != seen.end(); ++it)
    {
        const bsref val = *it;
        print_boardhash((bsref *)&val);
    }
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
    print_board(bs);
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

// TODO: this currently allows multiple adds of same blocker
void add_blocker(node *c, int id, int dir) {
    if (id == c->id) {
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

// clones the contents of board bsa into board bsb
void clone_bsref(bsref *bsb, bsref *bsa) {
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
    // XXX: array copy
    return seen.count(*bs) == 0;
}

// enum all possible moves of the piece at bidx (ie x, y)
//   and if they aren't already in seen, explore them (by ret 1)
bool predict_next(bsref *bs, bsref *c_out, uint8_t bidx, int x, int y) {
    int id = bs->s[bidx];
    printf("predict_next(%d @ %d, %d)\n", id, x, y);
    if (is_horiz(bs, bidx)) {
        if (x != 5) {
            for (int ix = 1; ix < 6; ix++) {
                if (bidx + ix >= 36) {
                    break;
                }
                if (bs->s[bidx + ix] == ID_BLANK) {
                    printf("horiz right\n");
                    make_move(c_out, id, x, y, x + 1, y);
                    if (is_new_hash(c_out)) {
                        // this is our new move
                        seen.insert(*c_out);
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
                printf("horiz left\n");
                make_move(c_out, id, x, y, x - 1, y);
                if (is_new_hash(c_out)) {
                    seen.insert(*c_out);
                    return 1;
                }
                make_move(c_out, id, x - 1, y, x, y);
            }
        }
    } else {
        if (y != 0) {
            if (bs->s[bidx - 6] == ID_BLANK) {
                printf("vert up\n");
                make_move(c_out, id, x, y, x, y - 1);
                if (is_new_hash(c_out)) {
                    seen.insert(*c_out);
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
                    printf("vert down\n");
                    make_move(c_out, id, x, y, x, y + 1);
                    if (is_new_hash(c_out)) {
                        printf("is new hash\n");
                        print_boardhash(c_out);
                        seen.insert(*c_out);
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

    print_depgraph(ss);
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

    clone_bsref(c_out, bs);
    printf("apply_heuristics(%d)\n", id);
    // consider free moves
    if (predict_next(bs, c_out, bidx, x, y)) {
        printf("found free move\n");
        *cid_out = id;
        return 1;
    }
    // consider blocker moves
    for (int i = 0; i < NUM_BLOCKERS; i++) {
        int b_id = curnode->blockers[i].id;
        if (b_id != ID_BLANK) {
            printf("found blocker move\n");
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
    printf(">>>>is>i>-c>>tio>>.. >>\n");
    printf("heuristic-ception... >>\n");
    print_depgraph(ss);
    fill_depgraph(bs, ss, curnode);
    print_depgraph(ss);
    for (int i = ID_P; i < 36; i++) {
        node *newnode = &ss->map[i];
        fill_depgraph(bs, ss, newnode);
        if (newnode->id != id) {
            printf("found extra move\n");
            if (apply_heuristics(bs, ss, newnode, c_out, cid_out)) {
                return 1;
            }
        }
    }
    print_depgraph(ss);

    return 0;
}

int is_solvable(bsref *init) {
    // the bottom of board_history is the initial board state.
    // the top of board_history is the current board state
    stack<bsref> board_history;
    // XXX: array copy
    board_history.push(*init);
    int steps = 0;
    // the id of the current piece to move
    int curid = ID_P;
    bsref curboard;
    memset(&curboard, 0x00, sizeof(curboard));
    clone_bsref(&curboard, init);
    while (steps < 0x20) {
        printf("\n[step %d] curid=%d\n", steps, curid);
        print_boardhash(&curboard);
        print_seen();
        print_board(&curboard);
        depgraph ss;
        node *curnode = &ss.map[curid];

        // is it solved right now?
        int px, py;
        find_piece(&curboard, ID_P, &px, &py);
        if (px == 4) {
            return 1;
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
            if (board_history.empty()) {
                printf("error in is_solvable (board_history empty)\n");
                return 0;
            }

            bsref top = board_history.top();
            unproductive.insert(top);
            board_history.pop();
            steps++;
            printf("dead end, board_history size is %d\n", board_history.size());
        } else {
            board_history.push(c);
            clone_bsref(&curboard, &c);
            steps++;
        }
    }
    printf("hit max step length -- give up\n");
    return 0;
}

int main() {
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

    if (is_solvable(&board_init)) {
        printf("solve\n");
    } else {
        printf("no solve\n");
    }
    return 0;
}
