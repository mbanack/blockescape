#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stack>
#include <set>

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

typedef struct node {
    uint8_t id;
    uint8_t init;
    // node ids blocking left and right respectively
    uint8_t l[5];
    uint8_t r[5];
} node;

#define NOMOVE 0xFF
#define FREE   0x00

#define RIGHT 1
#define LEFT  0

// convert board idx in 0 .. 35 to x, y in 0 .. 5
#define BIDX_TO_X(bidx) ((bidx) / 6)
#define BIDX_TO_Y(bidx) ((bidx) % 6)
// convert x,y to board idx
#define XY_TO_BIDX(x, y) ((6 * (y) + (x)))

typedef struct boardstate {
    // 2 lists, mapping ids and type
    // (or bitmasks)
    uint8_t id[36];
    uint8_t type[36];
} boardstate;

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

int get_id(boardstate *bs, int x, int y) {
    return bs->id[XY_TO_BIDX(x, y)];
}

typedef struct hash {
    // 4 bits per square * 36 => 144 bits
    // a square is only set if it is the top-left position of
    //   a piece, and is set to the id of that piece
    // so with 6 columns, each u32 is an entire row
    uint32_t b[5];
} hash;

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

typedef struct solvestate {
    // map[PIECE_IDX] = tree node
    node map[36];
} solvestate;

void add_blocker(node *c, int id, int right) {
    if (right) {
        for (int i = 0; i < 6; i++) {
            if (c->r[i] == FREE) {
                c->r[i] = id;
                return;
            }
        }
    } else {
        for (int i = 0; i < 6; i++) {
            if (c->l[i] == FREE) {
                c->l[i] = id;
                return;
            }
        }
    }
}

void nomove(node *c, int right) {
    for (int i = 0; i < 6; i++) {
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
    if (!ss->map[id].init) {
        printf("no cb for !init\n");
        return -1;
    }

    node c = ss->map[id];
    c.init = 1;
    // look in either direction of move axis and add all seen to list
    // TODO: this is just for horizontal moving pieces atm
    int x, y;
    find_piece(bs, id, &x, &y);
    if (x == -1) {
        return -1;
    }
    if (x == 0) {
        nomove(&c, LEFT);
    }
    if (x == 5) {
        nomove(&c, RIGHT);
    }
    for (int i = 0; i < 6; i++) {
        int other_id = bs->id[XY_TO_BIDX(i, y)];
        if (is_piece(bs, i, y) && id != other_id) {
            if (i < x) {
                add_blocker(&c, other_id, LEFT);
            } else {
                add_blocker(&c, other_id, RIGHT);
            }
        }
    }

    return 1;
}

// TODO: move to top/header
typedef struct move {
    int hash; // after applying this move
    int id;
    int old_x;
    int old_y;
    int new_x;
    int new_y;
} move;

set<int> seen;
set<int> unproductive;
// TODO: replace board_history with move struct {move desc, board hash (before?after?)}
//stack<int> board_history;

boardstate board_init;
// TODO: use this :)
boardstate curboard;
// the bottom of board_history is the initial board state.
stack<hash> board_history;
stack<move> move_history;

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
//   we only store the topleft.
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
                calc_blockers(&curboard, &ss, a->id);
            }
        }

        // pick one of the blockers as "our move" to try to solve

        int moved = 0;
        // prefer moving right
        for (int i = 0; i < 5; i++) {
            if (cur->r[i] == NOMOVE || cur->r[i] == FREE) {
                break;
            } else {
                cur = &ss.map[cur->r[i]];
                moved = 1;
            }
        }
        if (moved == 0) {
            for (int i = 0; i < 5; i++) {
                if (cur->l[i] == NOMOVE || cur->l[i] == FREE) {
                    break;
                } else {
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
        int horiz = is_horiz(cur->id);
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
        // TODO: call out to john board code
        make_move(&curboard, cur->id, new_x, new_y);

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
    board_init->id[12] = 0;
    board_init->id[11] = 1;
    board_init->id[10] = 2;
    board_init->id[14] = 3;
    board_init->id[18] = 4;
    board_init->id[22] = 5;
    // preload bottom of board_history
    hash init_hash;
    hash_board(board_init, &init_hash);
    board_history.push(init_hash);

    if (is_solvable()) {
        printf("solve\n");
    } else {
        printf("no solve\n");
    }
    return 0;
}
