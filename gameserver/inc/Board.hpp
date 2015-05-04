#ifndef BOARD_HPP_
#define BOARD_HPP_
#include <vector>
#include <fstream>
#include <iostream>
#include <map>
#include <SDL/SDL.h>
#include <string>
#include <set>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "../src/sio_client.h"
typedef std::vector<std::vector<int> > vvi;
typedef std::vector<int> vi;
typedef std::set<std::vector<std::vector<int> > > svvi;
const int BOARD_ROWS=6;
const int BOARD_COLS=6;
const int BOARD_CELL_SIZE=50;
class Board {
public:
    enum PIECE_TYPES { PIECE_PLAYER, EMPTY_SPACE, PIECE_HORIZONTAL2, 
        PIECE_HORIZONTAL3, PIECE_VERTICAL2, PIECE_VERTICAL3, 
        PIECE_TYPE_SIZE };
    Board(int width, int height); //unused
    Board(int width, int height, 
        const std::map<int, SDL_Surface *> &pieceGraphics);
    Board(int width, int height, 
        const std::map<int, SDL_Surface *> &pieceGraphics, 
        std::ifstream &f);
    Board(int width, int height, std::ifstream &f); //Text based
    int numFree(); //unused
    bool fullBoard(); //unused
    bool oneMoveSolution(); //unused
    bool oneMoveSolution(vvi board, int x, int y, int pieceType); //unused
    bool isCollision(int x, int y, int pieceType);
    bool isCollision(const vvi &board, int x, int y, int pieceType);
    void placePiece(int x, int y, int pieceType, uint8_t pid);
    void print(std::ostream &s);
    void move(int x, int y, int xp, int yp);
    void render(SDL_Surface *screen, SDL_Surface *background);
    void mouseDrag(SDL_Rect coordinates);
    void mouseRelease();
    void sendPieceLocations(websocketpp::server<websocketpp::config::asio> 
        & s, websocketpp::connection_hdl &h, int tid);
    void makeLotsOBoards();//unused
    //Board makeBoard(int numMoves);
    void printIds(std::ostream &s);
    void getIds(uint8_t ids[36]); //IDs only updated if you call move(..)!
private:
    void initializeIds();
    void makeLotsOBoards(vvi b, int x, int y, int type);
    std::multimap<SDL_Surface *, SDL_Rect> coordinatePieces();
    bool fullBoard(vvi board);
    void placePiece(vvi &board, uint8_t ids[36], int x, int y, int pieceType, uint8_t pid);
    bool fullBoard(vvi board, int x, int y, int pieceType);
    bool validMove(int x, int y, int xp, int yp);
    void grabFloatingPiece(SDL_Rect rect);
    bool checkCollision(SDL_Rect &rect, int pieceType, int xd, int yd);
    void removePiece(int index, vvi &board, uint8_t ids[36], 
        SDL_Rect &r, int &c, uint8_t &pid);
    int getFirstBlockIndex(int index);
    std::map<int, SDL_Surface *> pieceGraphics;
    vvi board;
    bool mouseDown;
    SDL_Rect mouseInitialRect;
    SDL_Rect floatingPieceRect;
    SDL_Rect floatingPieceInitial;
    SDL_Rect floatingPieceStuckRect;
    int floatingPieceType;
    uint8_t floatingPieceId;
    bool stopLeft, stopRight, stopUp, stopDown;
    std::string lastNetworkMessage;
    svvi lotsOBoards;
    uint8_t ids[36];
};
#endif
