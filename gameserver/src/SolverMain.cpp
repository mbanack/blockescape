#include <fstream>
#include "../inc/Board.hpp"
using namespace std;
int main(int argc, char **argv){
    ifstream f(argv[1]);
    Board b(6, 6, f);
    uint8_t ids[36];
    b.convertToIds(ids);
    ifstream g(argv[2]);
    int x, y, xp, yp;
    b.print(cout);
    while(g >> y){
        g >> x;
        g >> yp;
        g >> xp;
        cout << y << x << yp << xp << endl;
        b.move(x,y,xp,yp);
        b.print(cout);
    }
}
