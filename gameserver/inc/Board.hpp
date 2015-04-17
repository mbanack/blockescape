#ifndef BOARD_HPP_
#define BOARD_HPP_
#include <vector>
#include <fstream>
#include <iostream>
#include <map>
#include <SDL/SDL.h>
typedef std::vector<std::vector<char> > vvc;
typedef std::vector<char> vc;
const int BOARD_ROWS=6;
const int BOARD_COLS=6;
const int BOARD_CELL_SIZE=100;
class Board {
public:
    enum PIECE_TYPES { PIECE_PLAYER, EMPTY_SPACE, PIECE_HORIZONTAL2, 
        PIECE_HORIZONTAL3, PIECE_VERTICAL2, PIECE_VERTICAL3, 
        PIECE_TYPE_SIZE };
    Board(char width, char height, 
        const std::map<int, SDL_Surface *> &pieceGraphics);
    Board(char width, char height, 
        const std::map<int, SDL_Surface *> &pieceGraphics, 
        std::ifstream &f);
    Board(char width, char height, std::ifstream &f); //Text based
    char numFree();
    bool fullBoard();
    bool oneMoveSolution();
    bool oneMoveSolution(vvc board, int x, int y, char pieceType);
    bool isCollision(char x, char y, char pieceType);
    void placePiece(char x, char y, char pieceType);
    void print(std::ostream &s);
    void move(int x, int y, int xp, int yp);
    void render(SDL_Surface *screen, SDL_Surface *background);
private:
    std::multimap<SDL_Surface *, SDL_Rect> coordinatePieces();
    bool fullBoard(vvc board);
    void placePiece(vvc board, char x, char y, char pieceType);
    bool fullBoard(vvc board, int x, int y, char pieceType);
    bool validMove(int x, int y, int xp, int yp);
    std::map<int, SDL_Surface *> pieceGraphics;
    vvc board;
};
#endif
