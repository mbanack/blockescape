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

typedef struct boardstate {
    // 2 lists, mapping ids and type
    // (or bitmasks)
    uint8_t id[36];
    uint8_t type[36];
} boardstate;

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
        int other_id = get_id(x, y);
        if (is_piece(i, y) && id != other_id) {
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
stack<move> move_history;

// pop all hashes until we see the given hash
//   adding all popped hashes to an "unproductive" list
// TODO: also needs the boardstate as a parameter...
//       needs to "forward-wind from the beginning"
//       to go back to the previous board state
//  OR: apply "anti-moves"
void rewind(int hash) {
    int top = board_history.top();
    while (top != hash) {
        unproductive.insert(top);
        board_history.pop();
        top = board_history.top();
    }
}

int is_solvable(boardstate *bs) {
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
            calc_blockers(bs, &ss, ID_P);
        }

        // is it solved right now?
        int px, py;
        find_piece(bs, ID_P, &px, &py);
        if (px == 4) {
            return 1;
        }

        // walk the current set of blockers and continue graph generation
        for (int i = 0; i < 6; i++) {
            node *a = &ss.map[cur->l[i]];
            if (a->init == 0) {
                calc_blockers(bs, &ss, a->id);
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

        // TODO: alter the boardstate to reflect the move
        // ...

        // add new hash to "seen board states"
        // else rewind
        int hash = calc_hash(bs);
        if (seen.count(hash) != 0) {
            if (unproductive.count(hash) != 0) {
                rewind(hash);
            } else {
                int last_hash = board_history.top();
                unproductive.insert(last_hash);
                board_history.pop();
            }
        } else {
            seen.insert(hash);
            board_history.push(hash);
        }

        // TODO: calc new_x, new_y
        int horiz = is_horiz(cur->id);
        int old_x, old_y;
        find_piece(bs, cur->id, &old_x, &old_y);
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
        make_move(bs, cur->id, new_x, new_y);

        steps++;
    }
    printf("0xFFFF\n");
    return 0;
}

int main() {
    boardstate state;
    if(is_solvable(&state)) {
        printf("solve\n");
    } else {
        printf("no solve\n");
    }
    return 0;
}
