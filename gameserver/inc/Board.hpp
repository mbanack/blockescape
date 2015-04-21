#ifndef BOARD_HPP_
#define BOARD_HPP_
#include <vector>
#include <fstream>
#include <iostream>
#include <map>
#include <SDL/SDL.h>
#include <string>
#include "../src/sio_client.h"
typedef std::vector<std::vector<int> > vvi;
typedef std::vector<int> vi;
const int BOARD_ROWS=6;
const int BOARD_COLS=6;
const int BOARD_CELL_SIZE=100;
class Board {
public:
    enum PIECE_TYPES { PIECE_PLAYER, EMPTY_SPACE, PIECE_HORIZONTAL2, 
        PIECE_HORIZONTAL3, PIECE_VERTICAL2, PIECE_VERTICAL3, 
        PIECE_TYPE_SIZE };
    Board(int width, int height, 
        const std::map<int, SDL_Surface *> &pieceGraphics);
    Board(int width, int height, 
        const std::map<int, SDL_Surface *> &pieceGraphics, 
        std::ifstream &f);
    Board(int width, int height, std::ifstream &f); //Text based
    int numFree();
    bool fullBoard();
    bool oneMoveSolution();
    bool oneMoveSolution(vvi board, int x, int y, int pieceType);
    bool isCollision(int x, int y, int pieceType);
    void placePiece(int x, int y, int pieceType);
    void print(std::ostream &s);
    void move(int x, int y, int xp, int yp);
    void render(SDL_Surface *screen, SDL_Surface *background);
    void mouseDrag(SDL_Rect coordinates);
    void mouseRelease();
    void sendPieceLocations(sio::client &h);
private:
    std::multimap<SDL_Surface *, SDL_Rect> coordinatePieces();
    bool fullBoard(vvi board);
    void placePiece(vvi &board, int x, int y, int pieceType);
    bool fullBoard(vvi board, int x, int y, int pieceType);
    bool validMove(int x, int y, int xp, int yp);
    void grabFloatingPiece(SDL_Rect rect);
    bool checkCollision(SDL_Rect &rect, int pieceType, int xd, int yd);
    void removePiece(int index, vvi &board, SDL_Rect &r, int &c);
    int getFirstBlockIndex(int index);
    std::map<int, SDL_Surface *> pieceGraphics;
    vvi board;
    bool mouseDown;
    SDL_Rect mouseInitialRect;
    SDL_Rect floatingPieceRect;
    SDL_Rect floatingPieceInitial;
    int floatingPieceType;
    bool stopLeft, stopRight, stopUp, stopDown;
    std::string lastNetworkMessage;
};
#endif
