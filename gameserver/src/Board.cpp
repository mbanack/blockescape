#include "../inc/Board.hpp"
#include <cstdlib>
using namespace std;
void Board::move(int x, int y, int xp, int yp){
    if(!validMove(x, y, xp, yp)){
        cerr << "Invalid Move" << '\n';
        return;
    }
    char pieceType=board[y][x];
    char width=1;
    char height=1;
    if(pieceType==PIECE_HORIZONTAL2)
        width=2;
    if(pieceType==PIECE_HORIZONTAL3)
        width=3;
    if(pieceType==PIECE_VERTICAL2)
        height=2;
    if(pieceType==PIECE_VERTICAL3)
        height=3;
    for(int i=x;i<x+width;++i)
        board[y][i]=EMPTY_SPACE;
    for(int i=y;i<y+height;++i)
        board[i][x]=EMPTY_SPACE;
    for(int i=xp;i<xp+width;++i)
        board[yp][i]=pieceType;
    for(int i=yp;i<yp+height;++i)
        board[i][xp]=pieceType;
}
bool Board::validMove(int x, int y, int xp, int yp){
    char pieceType=board[y][x];
    if(pieceType != PIECE_PLAYER
        && pieceType != PIECE_HORIZONTAL2
        && pieceType != PIECE_HORIZONTAL3
        && pieceType != PIECE_VERTICAL2
        && pieceType != PIECE_VERTICAL3)
        return false;
    char width=1;
    char height=1;
    if(pieceType==PIECE_PLAYER||pieceType==PIECE_HORIZONTAL2)
        width=2;
    if(pieceType==PIECE_HORIZONTAL3)
        width=3;
    if(pieceType==PIECE_VERTICAL2)
        height=2;
    if(pieceType==PIECE_VERTICAL3)
        height=3;
    if(xp<0||yp<0||xp+width>board.size()||yp+height>board.size())
        return false;
    if(height==1&&y!=yp)
        return false;
    if(width==1&&x!=xp)
        return false;
    if(xp>x){
        for(int i=x+width;i<xp+width;++i){
            if(board[y][i]!=EMPTY_SPACE)
                return false;
        }
    }
    if(yp>y){
        for(int i=y+height;i<yp+height;++i){
            if(board[i][x]!=EMPTY_SPACE)
                return false;
        }
    }
    if(xp<x){
        for(int i=x-1;i>=xp;--i){
            if(board[y][i]!=EMPTY_SPACE)
                return false;
        }
    }
    if(yp<y){
        for(int i=y-1;i>=yp;--i){
            if(board[i][x]!=EMPTY_SPACE)
                return false;
        }
    }
    return true;
}
Board::Board(char width, char height){
    vector<char> row(width, EMPTY_SPACE);
    vvc ret(height, row);
    ret[(height+1)/2-1][0]=PIECE_PLAYER;
    ret[(height+1)/2-1][1]=PIECE_PLAYER;
    board=ret;
}
Board::Board(char width, char height, std::ifstream &f){
    for(int i=0;i<height;++i){
        string line;
        f >> line;
        vc row;
        for(string::iterator j=line.begin();j!=line.end();++j)
        {
            char s[2];
            s[0]=*j;
            s[1]='\0';
            row.push_back(atoi(s));
        }
        board.push_back(row);
    }
}
bool Board::isCollision(char x, char y, char pieceType){
    char width=1;
    char height=1;
    if(pieceType==PIECE_HORIZONTAL2)
        width=2;
    if(pieceType==PIECE_HORIZONTAL3)
        width=3;
    if(pieceType==PIECE_VERTICAL2)
        height=2;
    if(pieceType==PIECE_VERTICAL3)
        height=3;
    if(x+width>board[0].size() || y+height>board.size())
        return true;
    for(char i=y;i<y+height;++i){
        for(char j=x;j<x+width;++j){
            if(board[i][j] != EMPTY_SPACE)
                return true;
        }
    }
    return false;
}
bool Board::oneMoveSolution(vvc board, int x, int y, char pieceType){
    placePiece(board,x,y,pieceType);
    return oneMoveSolution();
}
bool Board::oneMoveSolution(){
    bool ret=true;
    for(int j=2;j<board[(board.size()+1)/2-1].size();++j){
        if(board[(board.size()+1)/2-1][j]!=EMPTY_SPACE)
            ret=false;
    }
    return ret;
}
char Board::numFree(){
    char ret=0;
    for(vvc::const_iterator i=board.begin();i!=board.end();++i){
        for(vc::const_iterator j=i->begin();j!=i->end();++j){
            if(*j!=EMPTY_SPACE)
                ++ret;
        }
    }
    return ret;
}
bool Board::fullBoard(vvc board, int x, int y, char pieceType){
    placePiece(board,x,y,pieceType);
    return fullBoard(board);
}
bool Board::fullBoard(){
    fullBoard(board);
}
bool Board::fullBoard(vvc board){
    bool ret=true;
    for(vvc::const_iterator i=board.begin();i!=board.end();++i){
        for(vector<char>::const_iterator j=i->begin();j!=i->end();++j){
            if(*j==EMPTY_SPACE)
                ret=false;
        }
    }
    return ret;
}
void Board::placePiece(char x, char y, char pieceType){
    placePiece(board,x,y,pieceType);
}
void Board::placePiece(vvc board, char x, char y, char pieceType){
    char width=1;
    char height=1;
    if(pieceType==PIECE_HORIZONTAL2)
        width=2;
    if(pieceType==PIECE_HORIZONTAL3)
        width=3;
    if(pieceType==PIECE_VERTICAL2)
        height=2;
    if(pieceType==PIECE_VERTICAL3)
        height=3;
    for(char i=y;i<y+height;++i){
        for(char j=x;j<x+width;++j){
            board[i][j]=pieceType;
        }
    }
}
void Board::print(ostream &s){
    for(int i=0;i<board.size();++i){
        for(int j=0;j<board[0].size();++j){
            s << int(board[i][j]);
        }
        s << '\n';
    }
}
