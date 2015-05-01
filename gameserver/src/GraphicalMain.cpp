#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "../inc/Board.hpp"
#include "sio_client.h"
using namespace std;
map<int, SDL_Rect> coordinates;
map<int, bool> down;
map<int, bool> newInput;
map<int, Board> boards;
set<int> ids;
map<int,SDL_Surface*> pieceGraphics;
string boardFile;
void networkNewBoard(sio::event &e){
    ifstream f(boardFile);
    string s = e.get_message()->get_string();
    int id = atoi(s.c_str());
    SDL_Rect r;
    Board b(6, 6, pieceGraphics, f);
    coordinates.insert(make_pair(id,r));
    down.insert(make_pair(id,false));
    newInput.insert(make_pair(id,true));
    boards.insert(make_pair(id,b));
    ids.insert(id);
}
void networkMouseInput(sio::event &e){
    stringstream s;
    s.str(e.get_message()->get_string());
    string downStr;
    string xStr;
    string yStr;
    string fromId;
    s >> fromId;
    s >> downStr;
    s >> xStr;
    s >> yStr;
    int tempId = atoi(fromId.c_str());
    if(newInput.count(tempId)>0)
        newInput.find(tempId)->second=true;
    if(atoi(downStr.c_str())==0){
        if(down.count(tempId)>0)
            down.find(tempId)->second = false;
    }
    else{
        if(down.count(tempId)>0)
            down.find(tempId)->second = true;
    }
    if(coordinates.count(tempId)>0){
        coordinates.find(tempId)->second.x = atoi(xStr.c_str());
        coordinates.find(tempId)->second.y = atoi(yStr.c_str());
    }
}
void sdlExit(){
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch(event.type){
        case SDL_QUIT:
            exit(0);
        }
    }
}
bool getMouseInput(bool &down, SDL_Rect &coordinates){
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch(event.type){
        case SDL_MOUSEBUTTONDOWN:
            if(event.button.button == SDL_BUTTON_LEFT){
                coordinates.x = event.button.x;
                coordinates.y = event.button.y;
                down = true;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if(event.button.button == SDL_BUTTON_LEFT)
                down = false;
            break;
        case SDL_MOUSEMOTION:
            coordinates.x=event.motion.x;
            coordinates.y=event.motion.y;
            break;
        case SDL_QUIT:
            return false;
        break;
        }
    }
    return true;
}
int main(int argc, char *argv[]){
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
    string tmp(argv[1]);
    boardFile=tmp;
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
    sio::client h;
    h.connect("http://127.0.0.1:9002");
    h.socket()->on("mouse input", &networkMouseInput);
    h.socket()->on("assign id html", &networkNewBoard);
    while(true){
        set<int> idsCopy = ids; //So that we don't modify while iterating
        for(set<int>::iterator i=idsCopy.begin();i!=idsCopy.end();++i){
            if(newInput.find(*i)->second){
                newInput.find(*i)->second = false;
                if(down.find(*i)->second==true)
                    boards.find(*i)->second.mouseDrag(coordinates.find(*i)->second);
                else
                {
                    boards.find(*i)->second.mouseRelease();
                    boards.find(*i)->second.printIds(cout);
                }
            }
            boards.find(*i)->second.sendPieceLocations(h, *i);
        }
        sdlExit();
    }
    SDL_FreeSurface(backgroundGraphic);
    SDL_FreeSurface(piecePlayerGraphic);
    SDL_FreeSurface(pieceHoriz2Graphic);
    SDL_FreeSurface(pieceHoriz3Graphic);
    SDL_FreeSurface(pieceVert2Graphic);
    SDL_FreeSurface(pieceVert3Graphic);
    IMG_Quit();
    SDL_Quit();
}
