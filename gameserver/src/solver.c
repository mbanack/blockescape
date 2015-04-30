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

#define NOMOVE 0xFF
#define FREE   0x00

#define RIGHT 1
#define LEFT  0

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
    printf("find_piece(%d)\n", id);
    *x = -1;
    *y = -1;

    for (int i = 0; i < 36; i++) {
        if (bs->id[i] == id && is_topleft(bs, i)) {
            *x = BIDX_TO_X(i);
            *y = BIDX_TO_Y(i);
            printf("found piece at %d, %d\n", *x, *y);
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

void make_move(boardstate *bs, int id, int old_x, int old_y, int new_x, int new_y) {
    int bidx = XY_TO_BIDX(old_x, old_y);
    if (is_horiz(bs, bidx)) {
        int width = calc_width(bs, id);
        insert_piece(bs, ID_BLANK, width, 1, old_x, old_y);
        insert_piece(bs, id, width, 1, new_x, new_y);
    } else {
        printf("!! NO VERT\n");
    }
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

void add_blocker(node *c, int id, int right) {
    printf("add_blocker( %d, %d)\n", id, right);
    if (right) {
        for (int i = 0; i < 5; i++) {
            if (c->r[i] == FREE) {
                c->r[i] = id;
                return;
            }
        }
    } else {
        for (int i = 0; i < 5; i++) {
            if (c->l[i] == FREE) {
                c->l[i] = id;
                return;
            }
        }
    }
}

void nomove(node *c, int right) {
    for (int i = 0; i < 5; i++) {
        if (right) {
            c->r[i] = NOMOVE;
        } else {
            c->l[i] = NOMOVE;
        }
    }
}

void fill_node(node *c, int id) {
    c->id = id;
    c->init = 0;
    for (int i = 0; i < 5; i++) {
        c->l[i] = FREE;
        c->r[i] = FREE;
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
    // TODO: this is just for horizontal moving pieces atm
    int x, y;
    find_piece(bs, id, &x, &y);
    if (x == -1) {
        return -1;
    }
    if (x == 0) {
        nomove(c, LEFT);
    }
    if (x == 5) {
        nomove(c, RIGHT);
    }
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
        for (int i = 0; i < 6; i++) {
            node *a = &ss.map[cur->l[i]];
            if (a->init == 0) {
                a->init = 1;
                calc_blockers(&curboard, &ss, a->id);
            }
        }

        // pick one of the blockers as "our move" to try to solve

        int moved = 0;
        // prefer moving right
        for (int i = 0; i < 5; i++) {
            // walk the list of blockers
            if (cur->r[i] == NOMOVE || cur->r[i] == FREE) {
                break;
            } else {
                printf("moveright map %d\n", i);
                cur = &ss.map[cur->r[i]];
                printf("cur becomes %d\n", cur->id);
                moved = 1;
            }
        }
        if (moved == 0) {
            for (int i = 0; i < 5; i++) {
                if (cur->l[i] == NOMOVE || cur->l[i] == FREE) {
                    break;
                } else {
                    printf("moveleft map %d\n", i);
                    cur = &ss.map[cur->l[i]];
                    moved = -1;
                }
            }
        }

        if (moved == 0) {
            printf("no moves to make... rewind\n");
            return 0;
        }


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
