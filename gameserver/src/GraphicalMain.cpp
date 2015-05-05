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
#include <cstdlib>
#include <ctime>
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
map<int, int> userBoardIndex;
map<string, int> userId;
set<int> ids;
map<string, string> userSalts1;
map<string, string> userSalts2;
string waitingId;
bool waitingPlayer = false;
websocketpp::connection_hdl waitingConnection;
map<string, websocketpp::connection_hdl> opponentConnection;
int identification=0;
const int TOTAL_BOARDS=2;
void login(server *s, websocketpp::connection_hdl hdl, 
    message_ptr msg, string username) {
    try {
        identification++;
        stringstream ss;
        ss << identification;
        int id = identification;
        ids.insert(id);
        if(userId.count(username)>0)
            userId.erase(userId.find(username));
        userId.insert(make_pair(username,id));
        string message("login");
        message+=ss.str();
        s->send(hdl, message, websocketpp::frame::opcode::text);
    }
    catch( const websocketpp::lib::error_code &e){}
}
void loginFail(server *s, websocketpp::connection_hdl hdl, 
    message_ptr msg) {
    try {
        string message("login");
        message+= "fail";
        s->send(hdl, message, websocketpp::frame::opcode::text);
    }
    catch( const websocketpp::lib::error_code &e){}
}
int levelIndexOkay(int index, const vector<int> &completed){
    if(index < 0)
        return 0;
    int thisLevel=0;
    int previousLevel=10;
    int ret = 0;
    int level = 0;
    for(int i = 0; i < TOTAL_BOARDS && i < completed.size() 
        && i <= index;++i){
        if((i+1)%10==0)
        {
            previousLevel=thisLevel;
            thisLevel=0;
        }
        if(completed[i] == 1)
            thisLevel+=1;
        if(previousLevel>=7){
            ret = i;
        }
    }
    return ret;
}
int levelOnePast(int index, const vector<int> &completed){
    int ret = 0;
    for(int i = 0; i < completed.size();++i){
        if(completed[i]==1)
            ret=i+1;
    }
    if(ret>TOTAL_BOARDS - 1)
        ret = TOTAL_BOARDS - 1;
    return ret;
}
void newBoard(server *s, websocketpp::connection_hdl hdl, 
    message_ptr msg, string username, int index, bool onePast,
    bool random) {
    SDL_Rect r;
    int id = userId.find(username)->second;
    Auth auth;
    if(onePast) {
        vector<int> completed = auth.getCompletedBoards(username);
        cout << "COMPLETED SIZE" << completed.size() << endl;
        index=levelOnePast(index,completed);
    }
    else if(random)
        index=rand()%TOTAL_BOARDS;
    else {
        vector<int> completed = auth.getCompletedBoards(username);
        cout << "COMPLETED SIZE" << completed.size() << endl;
        index=levelIndexOkay(index,completed);
    }
    stringstream ss;
    ss << index;
    string boardPath="../data/board";
    boardPath+=ss.str();
    ifstream f(boardPath);
    Board b(6, 6, f);
    f.close();
    if(coordinates.count(id)>0)
        coordinates.erase(coordinates.find(id));
    if(down.count(id)>0)
        down.erase(down.find(id));
    if(newInput.count(id)>0)
        newInput.erase(newInput.find(id));
    if(boards.count(id)>0)
        boards.erase(boards.find(id));
    if(userBoardIndex.count(id)>0)
        userBoardIndex.erase(userBoardIndex.find(id));
    coordinates.insert(make_pair(id,r));
    down.insert(make_pair(id,false));
    newInput.insert(make_pair(id,true));
    boards.insert(make_pair(id,b));
    userBoardIndex.insert(make_pair(id,index));
    try {
        string message("newboard");
        message+= ss.str();
        message+= " ";
        vector<int> completed = auth.getCompletedBoards(username);
        if(completed[index]==1)
            message+="completed";
        else
            message+="notcompleted";
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
    string multiplayer("multiplayer");
    string createUser("create user");
    string newBoardStr("newboard");
    if(msg->get_payload().substr(0,multiplayer.size())==
        multiplayer){
        try {
            string str(msg->get_payload().substr(multiplayer.size()));
            if(waitingPlayer){
                waitingPlayer=false;
                string message1("multiplayer");
                message1+=waitingId;
                string message2("multiplayer");
                message2+=str;
                opponentConnection.insert(make_pair(waitingId,hdl));
                opponentConnection.insert(make_pair(str,waitingConnection));
                s->send(hdl, message1, websocketpp::frame::opcode::text);
                s->send(waitingConnection, message2, websocketpp::frame::opcode::text);
            }
            else {
                waitingPlayer=true;
                waitingId = str;
                waitingConnection = hdl;
            }
        }
        catch( const websocketpp::lib::error_code &e){}
    }
    if(msg->get_payload().substr(0,newBoardStr.size())==
        newBoardStr){
        try {
            string str(msg->get_payload().substr(newBoardStr.size()));
            stringstream ss(str);
            string username;
            string action;
            string index;
            ss >> username;
            ss >> action;
            string constNext("next");
            string constPrevious("previous");
            string constJump("jump");
            string constOnePast("onepast");
            if(action==constNext||action==constPrevious||action==constJump){
                ss >> index;
                int indexInt = atoi(index.c_str());
                if(action==constNext)
                    ++indexInt;
                else if(action==constPrevious)
                    --indexInt;
                newBoard(s, hdl, msg, username, indexInt,
                    false, false);
            }
            else if(action==constOnePast)
                newBoard(s, hdl, msg, username, 0, true, false);
            else
                newBoard(s, hdl, msg, username, 0, false, true);
        }
        catch( const websocketpp::lib::error_code &e){}
    }
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
            if(auth.userExists(username)){
                loginFail(s, hdl, msg);
                return;
            }
            string salt1 = userSalts1.find(username)->second;
            if(auth.createUser(username, phash, salt1))
                login(s, hdl, msg, username);
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
            login(s, hdl, msg, username);
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
        if(opponentConnection.count(fromId)>0){
            boards.find(tempId)->second.sendPieceLocations(*s, opponentConnection.find(fromId)->second, tempId);
        }
        if(boards.find(tempId)->second.win()){
            string message = "win" + fromId;
            s->send(hdl, message, websocketpp::frame::opcode::text);
            if(opponentConnection.count(fromId)>0){
                s->send(opponentConnection.find(fromId)->second, message, websocketpp::frame::opcode::text);
                opponentConnection.erase(opponentConnection.find(fromId));
            }
            else{
                Auth auth;
                string username;
                for(map<string, int>::iterator it = userId.begin(); it !=
                    userId.end(); ++it) {
                    if(it->second==tempId){
                        username = it->first;
                    }
                }
                auth.updateCompletedBoards(userBoardIndex.find(tempId)->second,
                    username);
            }
        }
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
int main(int argc, char *argv[]){
    srand(time(0));
    startServer(webSocketServer);
    webSocketServer.run();
}

