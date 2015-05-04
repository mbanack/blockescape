#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "../inc/Board.hpp"
#include "../inc/Auth.hpp"
#include "sio_client.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>
#include <string>
using namespace std;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
typedef websocketpp::server<websocketpp::config::asio> server;
typedef server::message_ptr message_ptr;
server webSocketServer;
using namespace std;
map<int, SDL_Rect> coordinates;
map<int, bool> down;
map<int, bool> newInput;
map<int, Board> boards;
set<int> ids;
map<int,SDL_Surface*> pieceGraphics;
map<string, string> userSalts1;
map<string, string> userSalts2;
string boardFile;
void junk();
void junk2();
int identification=0;
void loginFail(server *s, websocketpp::connection_hdl hdl, 
    message_ptr msg) {
    try {
        string message("login");
        message+= "fail";
        s->send(hdl, message, websocketpp::frame::opcode::text);
    }
    catch( const websocketpp::lib::error_code &e){}
}
void login(server *s, websocketpp::connection_hdl hdl, 
    message_ptr msg) {
    ifstream f(boardFile);
    identification++;
    stringstream ss;
    ss << identification;
    int id = identification;
    SDL_Rect r;
    Board b(6, 6, pieceGraphics, f);
    coordinates.insert(make_pair(id,r));
    down.insert(make_pair(id,false));
    newInput.insert(make_pair(id,true));
    boards.insert(make_pair(id,b));
    ids.insert(id);
    try {
        string message("login");
        message+= ss.str();
        s->send(hdl, message, websocketpp::frame::opcode::text);
    }
    catch( const websocketpp::lib::error_code &e){}
}
void onMessage(server *s, websocketpp::connection_hdl hdl, 
    message_ptr msg) {
    string askSaltUsername("ask salt username");
    string askSaltsUsername("ask salts username");
    string assignIdHtml("assign id html");
    string mouseInput("mouse input");
    string loginStr("login");
    string createUser("create user");
    if(msg->get_payload().substr(0,askSaltUsername.size())==
        askSaltUsername){
        try {
            string message("get salt username");
            string str(msg->get_payload().substr(askSaltUsername.size()));
            Auth auth;
            string salt = auth.getSalt(str);
            userSalts1.insert(make_pair(str,salt));
            message+=salt;
            s->send(hdl, message, websocketpp::frame::opcode::text);
        }
        catch( const websocketpp::lib::error_code &e){}
    }
    else if(msg->get_payload().substr(0,askSaltsUsername.size())==
        askSaltsUsername){
        try {
            string message("get salts username");
            string str(msg->get_payload().substr(askSaltsUsername.size()));
            Auth auth;
            string salt1=auth.getSalt(str);
            string salt2=auth.genSalt();
            if(userSalts2.count(str)>0)
                userSalts2.erase(userSalts2.find(str));
            userSalts2.insert(make_pair(str, salt2));
            message+=salt1 + " " + salt2;
            s->send(hdl, message, websocketpp::frame::opcode::text);
        }
        catch( const websocketpp::lib::error_code &e){}
    }
    else if(msg->get_payload().substr(0,createUser.size())==
        createUser){
        try {
            string str(msg->get_payload().substr(createUser.size()));
            stringstream usernamePhashStream(str);
            string username;
            string phash;
            usernamePhashStream >> username;
            usernamePhashStream >> phash;
            Auth auth;
            string salt1 = userSalts1.find(username)->second;
            if(auth.createUser(username, phash, salt1))
                login(s, hdl, msg);
            else
                loginFail(s, hdl, msg);
        }
        catch( const websocketpp::lib::error_code &e){}
    }
    else if(msg->get_payload().substr(0,loginStr.size())==
        loginStr){
        string str(msg->get_payload().substr(loginStr.size()));
        stringstream userPassStream(str);
        string username;
        string phash;
        userPassStream >> username;
        userPassStream >> phash;
        string salt1;
        string salt2;
        Auth auth;
        salt1 = auth.getSalt(username);
        if(userSalts2.count(username)>0){
            salt2=userSalts2.find(username)->second;
        }
        else
            cerr << "Error: could not get salt" << endl;
        //Compare hashes
        if(auth.Authorize(username, salt2, phash))
            login(s, hdl, msg);
        else{
            loginFail(s, hdl, msg);
        }
    }
    else if(msg->get_payload().substr(0,mouseInput.size())==
        mouseInput){
        stringstream ss;
        ss.str(msg->get_payload().substr(mouseInput.size()));
        string downStr;
        string xStr;
        string yStr;
        string fromId;
        ss >> fromId;
        ss >> downStr;
        ss >> xStr;
        ss >> yStr;
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
        if(down.find(tempId)->second==true)
            boards.find(tempId)->second.mouseDrag(  
                coordinates.find(tempId)->second);
        else
            boards.find(tempId)->second.mouseRelease();
        boards.find(tempId)->second.sendPieceLocations(*s, hdl, tempId);
    }
}
void startServer(server &s){
    try {
        s.init_asio();
        s.set_message_handler(bind(&onMessage,&s,::_1,::_2));
        s.listen(9003);
        s.start_accept();
        s.run();
    }
    catch(...){
        cerr << "Error starting websocket server." << endl;
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
int main(int argc, char *argv[]){
    string tmp(argv[1]);
    boardFile=tmp;
    junk();
    //h.socket()->on("mouse input", &networkMouseInput);
    //h.socket()->on("assign id html", &networkNewBoard);
    startServer(webSocketServer);
    webSocketServer.run();
    atexit(junk2);
}

void junk2(){
    for(map<int, SDL_Surface *>::iterator it=pieceGraphics.begin();
        it != pieceGraphics.end(); ++it)
        SDL_FreeSurface(it->second);
    IMG_Quit();
    SDL_Quit();
}
//Junk but don't delete
void junk()
{
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
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
}
