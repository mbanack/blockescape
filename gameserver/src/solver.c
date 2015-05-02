#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stack>
#include <set>

#include "solver.h"

using namespace std;

// board is 6x6

// XXX: integration notes
// Board::move(oldx, oldy, x, y)
//        validmove(...)
//        isCollision()
//
// in addition to the move history stack, we need to store an initial
//   board state to be able to do any forward tracking
//
// TODO: helper sugars for "print current board state"
//         and "print current tree/table state"
//

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

boardstate board_init;
boardstate curboard;
set<hash> seen;
set<hash> unproductive;
// the bottom of board_history is the initial board state.
stack<hash> board_history;

uint8_t id_to_hex(int id) {
    if (id < 10) {
        return 48 + id; // decimal conv.
    } else {
        return 55 + id; // hex conv.
    }
}

void print_board() {
    printf("===\n");
    for (int i = 0; i < 36; i++) {
        printf("%c", id_to_hex(curboard.id[i]));
        if ((i + 1) % 6 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void insert_piece(boardstate *bs, int id, int width, int height, int new_x, int new_y) {
    int bidx = XY_TO_BIDX(new_x, new_y);
    if (width != 1) {
        for (int i = 0; i < width; i++) {
            bs->id[bidx + i] = id;
        }
    } else {
        for (int i = 0; i < height; i++) {
            bs->id[bidx + (6 * i)] = id;
        }
    }
}

int is_horiz(boardstate *bs, int idx) {
    int col = idx % 6;
    if (col != 0) {
        if (bs->id[idx - 1] == bs->id[idx]) {
            return 1;
        }
    }
    if (col != 5) {
        if (bs->id[idx + 1] == bs->id[idx]) {
            return 1;
        }
    }
    return 0;
}

// attempts to find a piece with given id, and returns its loc in x,y
void find_piece(boardstate *bs, int id, int *x, int *y) {
    *x = -1;
    *y = -1;

    for (int i = 0; i < 36; i++) {
        if (bs->id[i] == id && is_topleft(bs, i)) {
            *x = BIDX_TO_X(i);
            *y = BIDX_TO_Y(i);
            return;
        }
    }
}

int calc_width(boardstate *bs, int id) {
    int x, y;
    find_piece(bs, id, &x, &y);
    if (x != -1) {
        int w = 1;
        for (int bidx = XY_TO_BIDX(x, y); bidx < 36; bidx++) {
            if (bs->id[bidx] != id) {
                return w;
            }
            w++;
        }
    }
    return -1;
}

int calc_height(boardstate *bs, int id) {
    int x, y;
    find_piece(bs, id, &x, &y);
    if (x != -1) {
        int h = 1;
        for (int bidx = XY_TO_BIDX(x, y); bidx < 36; bidx+=6) {
            if (bs->id[bidx] != id) {
                return h;
            }
            h++;
        }
    }
    return -1;
}

void make_move(boardstate *bs, int id, int old_x, int old_y, int new_x, int new_y) {
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
void clone_boardstate(boardstate *bsa, boardstate *bsb) {

}

int is_piece(boardstate *bs, int x, int y) {
    if (bs->id[XY_TO_BIDX(x, y)] != ID_BLANK) {
        return 1;
    }
    return 0;
}

int is_topleft(boardstate *bs, int idx) {
    if (bs->id[idx] == ID_BLANK) {
        return 0;
    }
    int row = idx / 6;
    int col = idx % 6;
    if (row > 0) {
        if (bs->id[idx - 6] == bs->id[idx]) {
            return 0;
        }
    }
    if (col > 0) {
        if (bs->id[idx - 1] == bs->id[idx]) {
            return 0;
        }
    }
    return 1;
}

int get_id(boardstate *bs, int x, int y) {
    return bs->id[XY_TO_BIDX(x, y)];
}


bool operator==(const hash& l, const hash& r) {
    return l.b == r.b;
}

bool operator!=(const hash& l, const hash& r) {
    return l.b != r.b;
}

bool operator<(const hash& l, const hash& r) {
    return l.b < r.b;
}

bool operator>(const hash& l, const hash& r) {
    return l.b >= r.b;
}

// if we did some zero-collapsing, we could probably get a more
//   compact hash by adding a check for is_topleft(bs, i).
//   otherwise omitting the extra ids seems silly
//   because all we're doing is throwing out data
void hash_board(boardstate *bs, hash *h) {
    for (int i = 0; i < 5; i++) {
        h->b[i] = 0;
    }

    // do it row at a time
    for (int row = 0; row < 6; row++) {
        for (int i = 0; i < 6; i++) {
            int id = bs->id[XY_TO_BIDX(i, row)];
            h->b[row] |= id << (4 * (7 - i));
        }
    }
}

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
int calc_blockers(boardstate *bs, solvestate *ss, int id) {
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
    if (is_horiz(&curboard, id)) {
        for (int i = 0; i < 6; i++) {
            int other_id = bs->id[XY_TO_BIDX(i, y)];
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
            int other_id = bs->id[XY_TO_BIDX(x, i)];
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

// pop all hashes until we see the given hash
//   adding all popped hashes to an "unproductive" list
// TODO: also needs the boardstate as a parameter...
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

void clear_hash(hash *h) {
    for (int i = 0; i < 5; i++) {
        h->b[i] = 0;
    }
}

int is_null_hash(hash *h) {
    return h->b[0] == 0 &&
           h->b[1] == 0 &&
           h->b[2] == 0 &&
           h->b[3] == 0 &&
           h->b[4] == 0;
}


void predict_hash(uint8_t id, uint8_t dir, hash *next) {
    clear_hash(next);

    // enum all possible moves of that piece (given cur)
    //   and if they aren't already in seen, explore them
}

int consider_blockers(solvestate *ss, int *curid) {
    node *cur = &ss.map[*curid]
    for (int i = 0; i < NUM_BLOCKERS; i++) {
        if (cur->blockers[i].id != ID_BLANK) {
            hash predict;
            predict_hash(cur->blockers[i].id, cur->blockers[i].dir,
                         &predict);
            if (seen.count(predict) == 0) {
                // we haven't seen it, so try it
                *curid = cur->blockers[i].id;
                return 1;
            }
        }
    }
    return 0;
}

// consider "free moves" of cur
//   and moves of cur's blockers
// if we have a viable move
//   push the new hash onto board_history
//   update curboard
void apply_heuristics(boardstate *bs, node *curnode) {
    int id = curnode->id;
    int x, y;
    int bidx = XY_TO_BIDX(x, y);
    find_piece(bs, it, &x, &y);
    if (x == -1) {
        printf("error in apply_heuristics\n");
    }

    // check free moves
    if (is_horiz(bs, bidx)) {

    } else {

    }
}

// returns 1 if we have a productive free move, and update cur, etc
//   accordingly
int consider_free_moves(solvestate *ss, int *curid) {
    printf("stub consider_free_moves()");

    return 1;
}



    if (!consider_free_moves(&cur)) {
        if (!consider_blockers(&cur)) {
            return false;
        }
    }

    return false;
}


// iterate on a pre-loaded board_history
int is_solvable() {
    int steps = 0;
    node *cur;
    while (steps < 0xFFFF) {
        print_board();
        solvestate ss;
        // create solvestate with default node values
        //  (ie all pre-allocated)
        // we need to re-wipe the node solvestate if steps != 1
        //       because we just moved a piece.
        for (int i = 0; i < 36; i++) {
            fill_node(&ss.map[i], i);
        }
        if (steps == 0) {
            cur = &ss.map[ID_P];
            // starting from P (id 1)
            ss.map[ID_P].init = 1;
            calc_blockers(&curboard, &ss, ID_P);
        }

        // is it solved right now?
        int px, py;
        find_piece(&curboard, ID_P, &px, &py);
        if (px == 4) {
            return 1;
        }

        // walk the current set of blockers and continue graph generation
        for (int i = 0; i < NUM_BLOCKERS; i++) {
            node *a = &ss.map[cur->blockers[i].id];
            if (a->init == 0) {
                a->init = 1;
                calc_blockers(&curboard, &ss, a->id);
            }
        }

        // XXX: SCRUBA

        // now that we have generated the "blocking dependency graph" in ss
        // we try to pick a reasonable move based on heuristics
        // once a move has been exhausted, it is no longer considered,
        //   and we fall through to subsequent weighted heuristics

        // XXX: can we update the depgraph faster than wipe+regen?

        if (!apply_heuristics(&curboard, &cur)) {
            // this is a dead end, so pop it off the stack
            unproductive.push(board_history.top());
            board_history.pop();
        }

        // XXX: SCRUBA



        // add new hash to "seen board states"
        //   else rewind
        hash pre_hash;
        hash_board(&curboard, &pre_hash);

        // TODO: alter the boardstate to reflect the move
        // ...
        // TODO: calc new_x, new_y
        int horiz = is_horiz(&curboard, cur->id);
        int old_x, old_y;
        find_piece(&curboard, cur->id, &old_x, &old_y);
        int new_x, new_y;
        /*
        if (moved == -1) {
            if (horiz) {
                new_x = old_x - 1;
            } else {
                new_y = old_y - 1;
            }
        } else {
            if (horiz) {
                new_x = old_x + 1;
            } else {
                new_y = old_y + 1;
            }
        }
        */

        make_move(&curboard, cur->id, old_x, old_y, new_x, new_y);

        steps++;

        hash post_hash;

        // TODO: revisit... also reorg this whole func
        //   into helper modules :)
        if (seen.count(post_hash) != 0) {
            if (unproductive.count(post_hash) != 0) {
                // TODO: need to revisit rewind algo
                //   just because we looped doesnt mean the entire
                //   loop is unproductive
                // so do we just rewind one?
                rewind(post_hash);
            } else {
                hash last_hash = board_history.top();
                unproductive.insert(last_hash);
                board_history.pop();
            }
        } else {
            seen.insert(post_hash);
            board_history.push(post_hash);
        }
    }
    printf("0xFFFF\n");
    return 0;
}

int main() {
    // initialize global boardstate board_init with test board
    // TODO: given a Board, call Board::fillBoardstate(board_init)
    board_init.id[12] = 1;
    board_init.id[13] = 1;
    board_init.id[10] = 2;
    board_init.id[11] = 2;
    board_init.id[14] = 3;
    board_init.id[20] = 3;
    board_init.id[18] = 4;
    board_init.id[19] = 4;
    board_init.id[22] = 5;
    board_init.id[23] = 5;

    curboard = board_init;

    // preload bottom of board_history
    hash init_hash;
    hash_board(&board_init, &init_hash);
    board_history.push(init_hash);

    print_board();

    if (is_solvable()) {
        printf("solve\n");
    } else {
        printf("no solve\n");
    }
    return 0;
}
