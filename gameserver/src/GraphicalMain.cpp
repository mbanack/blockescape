#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <fstream>
#include <iostream>
#include "../inc/Board.hpp"
using namespace std;
int main(int argc, char **argv){
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
    ifstream f(argv[1]);
    map<int,SDL_Surface*> pieceGraphics;
    SDL_Surface *backgroundGraphic=IMG_Load("../data/board.png");
    SDL_Surface *piecePlayerGraphic=IMG_Load("../data/pieceplayer.png");
    SDL_Surface *pieceHoriz2Graphic=IMG_Load("../data/piecehorizontal2.png");
    SDL_Surface *pieceHoriz3Graphic=IMG_Load("../data/piecehorizontal3.png");
    SDL_Surface *pieceVert2Graphic=IMG_Load("../data/piecevertical2.png");
    SDL_Surface *pieceVert3Graphic=IMG_Load("../data/piecevertical3.png");
    pieceGraphics.insert(make_pair(Board::PIECE_PLAYER,piecePlayerGraphic));
    pieceGraphics.insert(make_pair(Board::PIECE_HORIZONTAL2,pieceHoriz2Graphic));
    pieceGraphics.insert(make_pair(Board::PIECE_HORIZONTAL3,pieceHoriz3Graphic));
    pieceGraphics.insert(make_pair(Board::PIECE_VERTICAL2,pieceVert2Graphic));
    pieceGraphics.insert(make_pair(Board::PIECE_VERTICAL3,pieceVert3Graphic));
    Board b(6, 6, pieceGraphics, f);
    SDL_Surface *s=SDL_SetVideoMode(800,600,32,0);
    b.render(s, backgroundGraphic);
    SDL_Flip(s);
    SDL_Delay(5000);
    SDL_FreeSurface(backgroundGraphic);
    SDL_FreeSurface(piecePlayerGraphic);
    SDL_FreeSurface(pieceHoriz2Graphic);
    SDL_FreeSurface(pieceHoriz3Graphic);
    SDL_FreeSurface(pieceVert2Graphic);
    SDL_FreeSurface(pieceVert3Graphic);
    IMG_Quit();
    SDL_Quit();
}
