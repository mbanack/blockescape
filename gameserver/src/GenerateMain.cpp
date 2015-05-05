#include <fstream>
#include "../inc/Board.hpp"
using namespace std;

int main(int argc, char **argv){
    for (int i = 0; i < 8; i++) {
        Board b();
        b.makeBoard(i);
        b.attemptSolve();
        int moves = b.numSolveMoves();
        if (b.solvable() && moves > 4) {
            printf("solved in %d moves\n", moves);
        }
    }
}
