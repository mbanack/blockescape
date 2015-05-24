/*
THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY
APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT
HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY
OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM
IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF
ALL NECESSARY SERVICING, REPAIR OR CORRECTION.
IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS
THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY
GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE
USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF
DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD
PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS),
EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGES.
*/
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
map<int, int> userHints;
map<string, int> userId;
set<int> ids;
map<string, string> userSalts1;
map<string, string> userSalts2;
string waitingId;
bool waitingPlayer = false;
websocketpp::connection_hdl waitingConnection;
map<string, websocketpp::connection_hdl> opponentConnection;
int identification=0;
const int TOTAL_BOARDS=1000;
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
        int hints = Auth::getInstance()->getHints(username);
        userHints.insert(make_pair(identification, hints));
        string message("login");
        message+=ss.str();
        s->send(hdl, message, websocketpp::frame::opcode::text);
        message = "num hints";
        stringstream ss2;
        ss2 << hints;
        cout << "num hints" << ss2.str() << hints;
        message += ss2.str();
        s->send(hdl, message, 
            websocketpp::frame::opcode::text);
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
void giveHints(server *s, websocketpp::connection_hdl hdl, 
    const vector<int> &hints, 
    int justCompletedIndex, const string &username, int tempId){
    if(hints[justCompletedIndex] == 1)
        return;
    int firstOfGroup = justCompletedIndex - (justCompletedIndex % 10);
    for(int i = firstOfGroup; i < firstOfGroup + 10; ++i) {
        if(hints[i] == 0 && i != justCompletedIndex)
            return;
    }
    if(userHints.count(tempId) == 0)
        userHints.insert(make_pair(tempId, 3));
    else
        userHints.find(tempId)->second += 3;
    Auth::getInstance()->updateHints(userHints.find(tempId)->second,
        username);
    string message = "num hints";
    stringstream ss2;
    ss2 << userHints.find(tempId)->second;
    cout << "num hints" << ss2.str();
    message += ss2.str();
    s->send(hdl, message, 
        websocketpp::frame::opcode::text);
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
        if(i>0&&i%10==0)
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
    Auth *auth = Auth::getInstance();
    if(onePast) {
        vector<int> completed = auth->getCompletedBoards(username);
        index=levelIndexOkay(levelOnePast(index,completed), completed);
    }
    else if(random)
        index=rand()%TOTAL_BOARDS;
    else {
        vector<int> completed = auth->getCompletedBoards(username);
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
        vector<int> completed = auth->getCompletedBoards(username);
        if(completed[index]==1)
            message+="completed ";
        else
            message+="notcompleted ";
        stringstream ss2;
        ss2 << b.getMinMoves();
        message += ss2.str();
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
    string undoStr("undo");
    string useHintStr("use hint");
    string numHintsStr("num hints");
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
            Auth *auth = Auth::getInstance();
            string salt = auth->getSalt(str);
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
            Auth *auth = Auth::getInstance();
            string salt1=auth->getSalt(str);
            string salt2=auth->genSalt();
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
            Auth *auth = Auth::getInstance();
            if(auth->userExists(username)){
                loginFail(s, hdl, msg);
                return;
            }
            string salt1 = userSalts1.find(username)->second;
            if(auth->createUser(username, phash, salt1))
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
        Auth *auth = Auth::getInstance();
        salt1 = auth->getSalt(username);
        if(userSalts2.count(username)>0){
            salt2=userSalts2.find(username)->second;
        }
        else
            cerr << "Error: could not get salt" << endl;
        //Compare hashes
        if(auth->Authorize(username, salt2, phash))
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
	    stringstream st;
	    st << boards.find(tempId)->second.getNumberOfMoves();
            string message = "win" + fromId + " " + st.str() + " " +
				boards.find(tempId)->second.numberOfSeconds; //add number of moves and time
            s->send(hdl, message, websocketpp::frame::opcode::text);
            if(opponentConnection.count(fromId)>0){
                s->send(opponentConnection.find(fromId)->second, message, websocketpp::frame::opcode::text);
                opponentConnection.erase(opponentConnection.find(fromId));
            }
            else{ 
                Auth *auth = Auth::getInstance();
                string username;
                for(map<string, int>::iterator it = userId.begin(); it !=
                    userId.end(); ++it) {
                    if(it->second==tempId){
                        username = it->first;
                    }
                }
                vector<int> completedBoards = 
                    auth->getCompletedBoards(username);
                giveHints(s, hdl, completedBoards, 
                    userBoardIndex.find(tempId)->
                    second, username, tempId);
                auth->updateCompletedBoards(
                    userBoardIndex.find(tempId)->second, username);
            }
        }
    }
    else if(msg->get_payload().substr(0,undoStr.size())==
        undoStr){
        try {
            stringstream ss;
            ss.str(msg->get_payload().substr(undoStr.size()));
            string fromId;
            ss >> fromId;
            int iFromId = atoi(fromId.c_str());
            if(boards.count(iFromId)>0)
            {
                if(boards.find(iFromId)->second.undo()){
                    string message = "undo0"; //disable undo button
                    s->send(hdl, message, 
                        websocketpp::frame::opcode::text);
                }
            }
        }
        catch( const websocketpp::lib::error_code &e){}
    }
    else if(msg->get_payload().substr(0,numHintsStr.size())==
        numHintsStr){
        try {
            stringstream ss;
            ss.str(msg->get_payload().substr(numHintsStr.size()));
            string fromId;
            ss >> fromId;
            int iFromId = atoi(fromId.c_str());
            string message = "num hints"; 
            if(userHints.count(iFromId)>0)
            {
                stringstream ss;
                ss << userHints.find(iFromId)->second;
                message += ss.str();
            }
            else
                message += '0';
            s->send(hdl, message, 
                websocketpp::frame::opcode::text);
        }
        catch( const websocketpp::lib::error_code &e){}
    }
    else if(msg->get_payload().substr(0,useHintStr.size())==
        useHintStr){
        try {
            stringstream ss;
            ss.str(msg->get_payload().substr(useHintStr.size()));
            string fromId;
            ss >> fromId;
            int iFromId = atoi(fromId.c_str());
            if(userHints.count(iFromId)>0)
            {
				if(userHints.find(iFromId)->second > 0)
                	userHints.find(iFromId)->second--;
                string username;
                for(map<string, int>::iterator it = userId.begin(); it !=
                    userId.end(); ++it) {
                    if(it->second==iFromId){
                        username = it->first;
                    }
                }
                Auth::getInstance()->updateHints(userHints.find(iFromId)->
                    second, username);
                string message = "num hints"; 
                stringstream ss;
                ss << userHints.find(iFromId)->second;
                message += ss.str();
                s->send(hdl, message, 
                    websocketpp::frame::opcode::text);
            }
        }
        catch( const websocketpp::lib::error_code &e){}
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

