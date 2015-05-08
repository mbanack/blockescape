//conflict hererereerere
#include "../inc/Board.hpp"
#include <cstdlib>
#include <sstream>
#include "solver.h"
using namespace std;
typedef websocketpp::server<websocketpp::config::asio> server;

void Board::attemptSolve() {
    bsref board_init;
    getIds(&board_init.s[0]);
    solve_result sr;
    //ai_solve(&board_init, &sr);
}


int Board::getMinMoves() {
    return minMoves;
}
bool Board::win(){
    return board[2][5]==PIECE_PLAYER;
}
void Board::printIds(ostream &s){
    for(int i = 0; i < BOARD_ROWS*BOARD_COLS;++i){
        cout << int(ids[i]);
        if((i+1)%BOARD_COLS==0)
            cout << endl;
    }
    cout << endl;
}
void Board::getIds(uint8_t *ids){
    for(int i = 0; i < 36; ++i){
        ids[i]=this->ids[i];
    }
}

void Board::initializeIds(){
    vvi board = this->board;
    uint8_t highestId=1; //0 is empty space 1 is player
    //Fill out board id array
    for(int i=0;i<BOARD_ROWS*BOARD_COLS;++i){
        int piece=board[i/BOARD_COLS][i%BOARD_COLS];
        if(piece==PIECE_HORIZONTAL2){
            ids[i]=++highestId;
            ids[i+1]=highestId;
            board[i/BOARD_COLS][i%BOARD_COLS]=-1;
            board[i/BOARD_COLS][(i+1)%BOARD_COLS]=-1;
        } 
        else if(piece==PIECE_HORIZONTAL3){
            ids[i]=++highestId;
            ids[i+1]=highestId;
            ids[i+2]=highestId;
            board[i/BOARD_COLS][i%BOARD_COLS]=-1;
            board[i/BOARD_COLS][(i+1)%BOARD_COLS]=-1;
            board[i/BOARD_COLS][(i+2)%BOARD_COLS]=-1;
        } 
        else if(piece==PIECE_VERTICAL2){
            ids[i]=++highestId;
            ids[i+BOARD_COLS]=highestId;
            board[i/BOARD_COLS][i%BOARD_COLS]=-1;
            board[(i+BOARD_COLS)/BOARD_COLS][i%BOARD_COLS]=-1;
        }  
        else if(piece==PIECE_VERTICAL3){
            ids[i]=++highestId;
            ids[i+BOARD_COLS]=highestId;
            ids[i+BOARD_COLS*2]=highestId;
            board[i/BOARD_COLS][i%BOARD_COLS]=-1;
            board[(i+BOARD_COLS)/BOARD_COLS][i%BOARD_COLS]=-1;
            board[(i+2*BOARD_COLS)/BOARD_COLS][i%BOARD_COLS]=-1;
        } 
        else if(piece==EMPTY_SPACE){
            ids[i]=0;
        }
        else if(piece==PIECE_PLAYER){
            ids[i]=1;
            ids[i+1]=1;
            board[i/BOARD_COLS][i%BOARD_COLS]=-1;
            board[i/BOARD_COLS][(i+1)%BOARD_COLS]=-1;
        }
    }
}

void Board::sendPieceLocations(server &ser, websocketpp::connection_hdl &hdl, int tid){
    stringstream s;
    vector<SDL_Rect> pieces = coordinatePieces();
    int numPieces = 0;
    for(vector<SDL_Rect>::const_iterator 
        i=pieces.begin(); i!=pieces.end();++i) {
        if(i->w!=i->h)
            numPieces++;
        
    }
    s << tid << " " << numPieces + 1; //+ 1 is floating piece
    try {
        string m("num pieces");
        m+=s.str();
        ser.send(hdl, m, websocketpp::frame::opcode::text);
    }
    catch( const websocketpp::lib::error_code &e){}
    s.str("");
    SDL_Rect r;
    for(vector<SDL_Rect>::const_iterator i=pieces.begin();
        i!=pieces.end();++i) {
        r=*i;
        if(r.w != r.h)
            s << tid << " " << board[r.y/BOARD_CELL_SIZE][r.x/BOARD_CELL_SIZE] << " " 
                << r.x << " " << r.y << " " << r.w << " " << r.h << " ";
    }
    r=floatingPieceRect;
    if(floatingPieceType!=EMPTY_SPACE) {
    s << tid << " " << floatingPieceType << " " << r.x << " " << r.y << " " 
        << r.w << " " << r.h << " ";
    }
        
    string message=s.str();
    try {
        string m("draw pieces");
        m+=message;
        ser.send(hdl, m, websocketpp::frame::opcode::text);
    }
    catch( const websocketpp::lib::error_code &e){}
    lastNetworkMessage=message;
}
void Board::mouseDrag(SDL_Rect rect){
    SDL_Rect last = floatingPieceRect;
    int xd=0;
    int yd=0;
    if(mouseDown){
        if(floatingPieceType==PIECE_HORIZONTAL2
            ||floatingPieceType==PIECE_HORIZONTAL3
            ||floatingPieceType==PIECE_PLAYER){
            xd=rect.x-mouseInitialRect.x;
            if((stopRight && xd>0) || (stopLeft && xd < 0)); //ps intended
            else
                floatingPieceRect.x=floatingPieceInitial.x + 
                    (rect.x - mouseInitialRect.x);
        }
        else if(floatingPieceType==PIECE_VERTICAL2||
            floatingPieceType==PIECE_VERTICAL3){
            yd=rect.y-mouseInitialRect.y;
            if((stopUp && yd<0) || (stopDown && yd>0)); //ps intended
            else
                floatingPieceRect.y=floatingPieceInitial.y + 
                    (rect.y - mouseInitialRect.y);
        }
    }
    else{
        if(board[rect.y/BOARD_CELL_SIZE][rect.x/BOARD_CELL_SIZE]
            == EMPTY_SPACE)
            return;
        mouseDown=true;
        mouseInitialRect=rect;
        grabFloatingPiece(rect);
        last=floatingPieceRect;
        floatingPieceInitial=floatingPieceRect;
    }
    if(mouseDown && checkCollision(floatingPieceRect, floatingPieceType, xd, yd)){
        if(!stopRight && !stopLeft && !stopDown && !stopUp)
            floatingPieceStuckRect=rect;
        if(xd>0)
            stopRight=true;
        else
            stopRight=false;
        if(xd<0)
            stopLeft=true;
        else
            stopLeft=false;
        if(yd>0)
            stopDown=true;
        else
            stopDown=false;
        if(yd<0)
            stopUp=true;
        else
            stopUp=false;
    }
    if(xd>0&&rect.x<floatingPieceStuckRect.x)
        stopRight=false;
    if(xd<0&&rect.x>floatingPieceStuckRect.x)
        stopLeft=false;
    if(yd>0&&rect.y<floatingPieceStuckRect.y)
        stopDown=false;
    if(yd<0&&rect.y>floatingPieceStuckRect.y)
        stopUp=false;
}
void Board::grabFloatingPiece(SDL_Rect rect){
    int index = rect.y/BOARD_CELL_SIZE*BOARD_COLS + rect.x/BOARD_CELL_SIZE;
    int newIndex=getFirstBlockIndex(index);
    removePiece(newIndex,board,ids,floatingPieceRect,floatingPieceType,
        floatingPieceId);
    if(newIndex-index==1)
        floatingPieceRect.x+=BOARD_CELL_SIZE;
    if(newIndex-index==2)
        floatingPieceRect.x+=2*BOARD_CELL_SIZE;
    if(newIndex-index==BOARD_COLS)
        floatingPieceRect.y+=BOARD_CELL_SIZE;
    if(newIndex-index==2*BOARD_COLS)
        floatingPieceRect.y+=2*BOARD_CELL_SIZE;
}
//Before running this make sure floating piece not still on board
bool Board::checkCollision(SDL_Rect &rect, int pieceType, int xd, int yd){
    vector<SDL_Rect> pieces = coordinatePieces();
    for(vector<SDL_Rect>::const_iterator i=pieces.begin();
        i != pieces.end(); ++i){
            //Skip empty space pieces, which are square
            if(i->w == i->h)
                continue;
            if((rect.x+rect.w>i->x 
                && rect.x<i->x+i->w)
                ||(rect.x < i->x + i->w
                    && rect.x > i->x)){
                if((rect.y+rect.h>i->y 
                    && rect.y<i->y+i->h)
                || (rect.y < i->y + i->h
                    && rect.y > i->y)){
                    if(xd>0)
                        rect.x=i->x-rect.w;
                    else if(xd<0)
                        rect.x=i->x+i->w;
                    if(yd>0)
                        rect.y=i->y-rect.h;
                    else if(yd<0)
                        rect.y=i->y+i->h;
                    return true;
                }
            }
    }
    if(rect.x<0){
        rect.x=0;
        return true;
    }
    if(rect.y<0){
        rect.y=0;
        return true;
    }
    if(rect.x+rect.w>BOARD_COLS*BOARD_CELL_SIZE){
        rect.x=BOARD_COLS*BOARD_CELL_SIZE-rect.w;
        return true;
    }
    if(rect.y+rect.h>BOARD_ROWS*BOARD_CELL_SIZE){
        rect.y=BOARD_ROWS*BOARD_CELL_SIZE-rect.h;
        return true;
    }
    return false;
}
void Board::mouseRelease(){
    if(mouseDown){
        mouseDown=false;
        int x = floatingPieceRect.x/BOARD_CELL_SIZE;
        int y = floatingPieceRect.y/BOARD_CELL_SIZE;
        placePiece(x,y,floatingPieceType,floatingPieceId);
        floatingPieceType = EMPTY_SPACE;
        floatingPieceId = 0;
        stopLeft = false;
        stopRight = false;
        stopUp = false;
        stopDown = false;
    }
}
//Doesn't work if i not first section of piece
void Board::removePiece(int i, vvi &board,
    uint8_t ids[36], SDL_Rect &rect, int &p, uint8_t &pid){
    int piece=board[i/BOARD_COLS][i%BOARD_COLS];
    p=piece;
    pid=ids[i];
    rect.x = i%BOARD_COLS*BOARD_CELL_SIZE;
    rect.y = i/BOARD_COLS*BOARD_CELL_SIZE;
    rect.w = BOARD_CELL_SIZE;
    rect.h = BOARD_CELL_SIZE;
    if(piece==PIECE_HORIZONTAL2||piece==PIECE_PLAYER){
        ids[i]=0;
        ids[i+1]=0;
        board[i/BOARD_COLS][i%BOARD_COLS]=EMPTY_SPACE;
        board[i/BOARD_COLS][(i+1)%BOARD_COLS]=EMPTY_SPACE;
        rect.w*=2;
    } 
    else if(piece==PIECE_HORIZONTAL3){
        ids[i]=0;
        ids[i+1]=0;
        ids[i+2]=0;
        board[i/BOARD_COLS][i%BOARD_COLS]=EMPTY_SPACE;
        board[i/BOARD_COLS][(i+1)%BOARD_COLS]=EMPTY_SPACE;
        board[i/BOARD_COLS][(i+2)%BOARD_COLS]=EMPTY_SPACE;
        rect.w*=3;
    } 
    else if(piece==PIECE_VERTICAL2){
        ids[i]=0;
        ids[i+BOARD_COLS]=0;
        board[i/BOARD_COLS][i%BOARD_COLS]=EMPTY_SPACE;
        board[(i+BOARD_COLS)/BOARD_COLS][i%BOARD_COLS]=EMPTY_SPACE;
        rect.h*=2;
    }  
    else if(piece==PIECE_VERTICAL3){
        ids[i]=0;
        ids[i+BOARD_COLS]=0;
        ids[i+2*BOARD_COLS]=0;
        board[i/BOARD_COLS][i%BOARD_COLS]=EMPTY_SPACE;
        board[(i+BOARD_COLS)/BOARD_COLS][i%BOARD_COLS]=EMPTY_SPACE;
        board[(i+2*BOARD_COLS)/BOARD_COLS][i%BOARD_COLS]=EMPTY_SPACE;
        rect.h*=3;
    } 
}
int Board::getFirstBlockIndex(int index){
    vvi board=this->board; //Local copy since we don't want to overwrite
    int piece = board[index/BOARD_CELL_SIZE][index%BOARD_CELL_SIZE];
    uint8_t ids[36];
    uint8_t pid=0;
    for(int i=0;i<BOARD_COLS*BOARD_ROWS;++i){
        SDL_Rect rect;
        removePiece(i, board, ids, rect, piece, pid);
        if(board[index/BOARD_COLS][index%BOARD_COLS]==EMPTY_SPACE)
            return i;
    }
    return 0; //Err
}
vector<SDL_Rect> Board::coordinatePieces(){
    vector<SDL_Rect> ret;
    vvi board=this->board; //Local copy since we don't want to overwrite
    uint8_t ids[36];
    uint8_t pid=0;
    for(int i=0;i<BOARD_COLS*BOARD_ROWS;++i){
        SDL_Rect rect;
        int piece;
        removePiece(i, board, ids, rect, piece, pid);
        ret.push_back(rect);
    }
    return ret;
}
void Board::move(int x, int y, int xp, int yp){
    if(!validMove(x, y, xp, yp)){
        cerr << "Invalid Move" << '\n';
        return;
    }
    int pieceType=board[y][x];
    int id = ids[y*BOARD_COLS+x];
    int width=1;
    int height=1;
    if(pieceType==PIECE_HORIZONTAL2)
        width=2;
    if(pieceType==PIECE_HORIZONTAL3)
        width=3;
    if(pieceType==PIECE_VERTICAL2)
        height=2;
    if(pieceType==PIECE_VERTICAL3)
        height=3;
    for(int i=x;i<x+width;++i){
        board[y][i]=EMPTY_SPACE;
        ids[y*BOARD_COLS+i]=0;
    }
    for(int i=y;i<y+height;++i){
        board[i][x]=EMPTY_SPACE;
        ids[i*BOARD_COLS+x]=0;
    }
    for(int i=xp;i<xp+width;++i){
        board[yp][i]=pieceType;
        ids[yp*BOARD_COLS+i]=id;
    }
    for(int i=yp;i<yp+height;++i){
        board[i][xp]=pieceType;
        ids[i*BOARD_COLS+xp]=id;
    }
}
bool Board::validMove(int x, int y, int xp, int yp){
    int pieceType=board[y][x];
    if(pieceType != PIECE_PLAYER
        && pieceType != PIECE_HORIZONTAL2
        && pieceType != PIECE_HORIZONTAL3
        && pieceType != PIECE_VERTICAL2
        && pieceType != PIECE_VERTICAL3)
        return false;
    int width=1;
    int height=1;
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
Board::Board(int width, int height):mouseDown(false),
    stopLeft(false), stopRight(false), stopUp(false), stopDown(false),
    floatingPieceType(EMPTY_SPACE), floatingPieceId(0){
    vector<int> row(width, EMPTY_SPACE);
    vvi ret(height, row);
    ret[(height+1)/2-1][0]=PIECE_PLAYER;
    ret[(height+1)/2-1][1]=PIECE_PLAYER;
    board=ret;
    initializeIds();
}
Board::Board(int width, int height, std::ifstream &f):mouseDown(false),
    stopLeft(false), stopRight(false), stopUp(false), stopDown(false),
    floatingPieceType(EMPTY_SPACE),floatingPieceId(0),board(){
    string minMovesStr;
    f >> minMovesStr;
    minMoves = atoi(minMovesStr.c_str());
    for(int i=0;i<height;++i){
        string line;
        f >> line;
        vi row;
        for(string::iterator j=line.begin();j!=line.end();++j)
        {
            char s[2];
            s[0]=*j;
            s[1]='\0';
            row.push_back(atoi(s));
        }
        board.push_back(row);
    }
    initializeIds();
}
bool Board::isCollision(int x, int y, int pieceType){
    return isCollision(board, x, y, pieceType);
}
bool Board::isCollision(const vvi &board, int x, int y, int pieceType){
    int width=1;
    int height=1;
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
    for(int i=y;i<y+height;++i){
        for(int j=x;j<x+width;++j){
            if(board[i][j] != EMPTY_SPACE)
                return true;
        }
    }
    return false;
}
bool Board::oneMoveSolution(vvi board, int x, int y, int pieceType){
    uint8_t pid=0;
    uint8_t ids[36];
    placePiece(board,ids,x,y,pieceType,pid);
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
int Board::numFree(){
    int ret=0;
    for(vvi::const_iterator i=board.begin();i!=board.end();++i){
        for(vi::const_iterator j=i->begin();j!=i->end();++j){
            if(*j!=EMPTY_SPACE)
                ++ret;
        }
    }
    return ret;
}
bool Board::fullBoard(vvi board, int x, int y, int pieceType){
    uint8_t pid=0;
    uint8_t ids[36];
    placePiece(board,ids,x,y,pieceType,pid);
    return fullBoard(board);
}
bool Board::fullBoard(){
    return fullBoard(board);
}
bool Board::fullBoard(vvi board){
    bool ret=true;
    for(vvi::const_iterator i=board.begin();i!=board.end();++i){
        for(vector<int>::const_iterator j=i->begin();j!=i->end();++j){
            if(*j==EMPTY_SPACE)
                ret=false;
        }
    }
    return ret;
}
void Board::placePiece(int x, int y, int pieceType, uint8_t pid){
    placePiece(board,ids,x,y,pieceType,pid);
}
void Board::placePiece(vvi &board, uint8_t ids[36], int x, int y, int pieceType, uint8_t pid){
    int width=1;
    int height=1;
    if(pieceType==PIECE_HORIZONTAL2||pieceType==PIECE_PLAYER)
        width=2;
    if(pieceType==PIECE_HORIZONTAL3)
        width=3;
    if(pieceType==PIECE_VERTICAL2)
        height=2;
    if(pieceType==PIECE_VERTICAL3)
        height=3;
    for(int i=y;i<y+height;++i){
        for(int j=x;j<x+width;++j){
            ids[i*BOARD_COLS+j]=pid;
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
