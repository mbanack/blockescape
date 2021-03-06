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
#include "../inc/Auth.hpp"
#include <string>
#include <sstream>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <sodium.h>
#include "mysql_driver.h"
using namespace std;

const char *DB_USER = "blockescape";
const char *DB_PASS = "horthownavlokum3";

Auth *Auth::instance = NULL;
Auth::Auth() 
{
    driver = get_driver_instance();
    con = 
        driver->connect("localhost", DB_USER, DB_PASS);
}
Auth *Auth::getInstance(){
    if(!instance)
        instance = new Auth();
    return instance;
}
Auth::~Auth(){
    delete con;
}
void Auth::updateHints(int numHints, const string &username){
    con->setSchema("users");
    sql::Statement *s = con->createStatement();
    stringstream ss;
    ss << numHints;
    string update("UPDATE users SET hints = \"" + ss.str() + "\" WHERE username = \"" + username + "\";");
    s->execute(update.c_str());
    delete s;
}
int Auth::getHints(const string &username) {
    con->setSchema("users");
    sql::Statement *s = con->createStatement();
    string select("SELECT hints FROM users WHERE username = \"" + username
        + "\";");
    sql::ResultSet *r = s->executeQuery(select.c_str());
    int ret = 0;
    if(r->next())
        ret=r->getInt("hints");
    delete s;
    delete r;
    return ret;
}
void Auth::updateCompletedBoards(int index, const string &username){
    con->setSchema("users");
    sql::Statement *s = con->createStatement();
    string select("SELECT levels FROM users WHERE username = \"" + username
        + "\";");
    sql::ResultSet *r = s->executeQuery(select.c_str());
    string levels;
    if(r->next())
    {
        levels=r->getString("levels");
        int i = 0;
        for(string::iterator it=levels.begin();it!=levels.end();++it){
            if(i==index)
                *it='1';
            i++;
        }
    }
    delete s;
    s = con->createStatement();
    string update("UPDATE users SET levels = \"" + levels + "\" WHERE username = \"" + username + "\";");
    s->execute(update.c_str());
    delete s;
    delete r;
}
std::vector<int> Auth::getCompletedBoards(const std::string &username){
    con->setSchema("users");
    sql::Statement *s = con->createStatement();
    string select("SELECT levels FROM users WHERE username = \"" + username
        + "\";");
    sql::ResultSet *r = s->executeQuery(select.c_str());
    vector<int> ret;
    if(r->next())
    {
        string levels=r->getString("levels");
        for(string::iterator it=levels.begin();it!=levels.end();++it){
            char tmp[2] = { *it, '\0'};
            ret.push_back(atoi(tmp));
        }
    }
    delete s;
    delete r;
    return ret;
}
bool Auth::userExists(const string &username){
        driver->connect("localhost", DB_USER, DB_PASS);
    con->setSchema("users");
    sql::Statement *s = con->createStatement();
    string select("SELECT * FROM users WHERE username = \"" + username
        + "\";");
    sql::ResultSet *r = s->executeQuery(select.c_str());
    if(r->next())
    {
        delete s;
        delete r;
        return true;
    }
    delete s;
    delete r;
    return false;
}
bool Auth::createUser(const string &username,
    const string &password, const string &salt){
    bool success=true;
    con->setSchema("users");
    sql::Statement *s = con->createStatement();
    string insert("INSERT INTO users (username, password, salt, levels, hints) VALUES (\"");
    insert += username;
    insert += "\", \"";
    insert += password;
    insert += "\", \"";
    insert += salt;
    insert += "\", \"";
    for(int i=0;i<3000;++i)
        insert.push_back('0');
    insert += "\", 0";
    insert += ");";
    try{
        s->execute(insert.c_str());
    }
    catch(sql::SQLException e){
        success = false;
    }
    delete s;
    return success;
}
bool Auth::Authorize(const string &username, const string &salt2,
    const string &password){
    con->setSchema("users");
    sql::Statement *s = con->createStatement();
    string select("SELECT password FROM users WHERE username = \"");
    select += username;
    select += "\";";
    sql::ResultSet *r = s->executeQuery(select.c_str());
    string dbpassword; 
    if(r->next()){
        dbpassword=r->getString("password");
    }
    string hashIn1 = salt2 + dbpassword;
    //new
    unsigned char out[crypto_hash_sha256_BYTES];
    crypto_hash_sha256(out, ((const unsigned char *)hashIn1.c_str()), hashIn1.size());
    char out2[crypto_hash_sha256_BYTES*2+1];
    sodium_bin2hex(out2, crypto_hash_sha256_BYTES * 2 + 1,
                     out, crypto_hash_sha256_BYTES);
    string finalHash(out2);
    //end new
    delete s;
    delete r;
    return finalHash == password;
}
string Auth::getSalt(const string &username){
    //sql::Driver *driver = get_driver_instance();
    //sql::Connection *con = 
     //   driver->connect("localhost", DB_USER, DB_PASS);
    con->setSchema("users");
    sql::Statement *s = con->createStatement();
    string select("SELECT salt FROM users WHERE username = \"");
    select += username;
    select += "\";";
    sql::ResultSet *r = s->executeQuery(select.c_str());
    string salt=genSalt();
    while(r->next()){
        salt=r->getString("salt");
    }
    delete s;
    delete r;
    return salt;
}
string Auth::genSalt(){
    unsigned char out[32];
    randombytes_buf(out, 32);
    char out2[32*2+1];
    sodium_bin2hex(out2, 32 * 2 + 1, out, 32);
    string s(out2);
    return s;
}

Highscore Auth::getHighscore(int boardId)
{
    stringstream ss;
    ss << boardId;
    con->setSchema("users");
    sql::Statement *s = con->createStatement();
    string select("SELECT * FROM highscore WHERE bid = " + ss.str() + ";");
    sql::ResultSet *r = s->executeQuery(select.c_str());
    Highscore ret;
    if(r->next())
    {
        string user1=r->getString("topUser");
        string user2=r->getString("topUser2");
        string user3=r->getString("topUser3");

        int moves1=atoi(r->getString("topMoves").c_str());
        int moves2=atoi(r->getString("topMoves2").c_str());
        int moves3=atoi(r->getString("topMoves3").c_str());

        string time1=r->getString("topTime");
        string time2=r->getString("topTime2");
        string time3=r->getString("topTime3");

        ret.topUser = user1;
        ret.topUser2 = user2;
        ret.topUser3 = user3;

        ret.topMoves = moves1;
        ret.topMoves2 = moves2;
        ret.topMoves3 = moves3;

        ret.topTime = time1;
        ret.topTime2 = time2;
        ret.topTime3 = time3;    
 
    }
    delete s;
    delete r;
    return ret;
}

void Auth::updateHighscore(int boardId, Highscore h)
{
    con->setSchema("users");
    sql::Statement *s = con->createStatement();
    stringstream sstopmoves1;
    stringstream sstopmoves2;
    stringstream sstopmoves3;
    sstopmoves1 << h.topMoves;
    sstopmoves2 << h.topMoves2;
    sstopmoves3 << h.topMoves3;
    string topmoves1 = sstopmoves1.str();
    string topmoves2 = sstopmoves2.str();
    string topmoves3 = sstopmoves3.str();
    stringstream ss;
    ss << boardId;
    string boardIdStr = ss.str();
    stringstream st;
    cout << "GOT TO A" << endl;
    st << "UPDATE highscore SET"
       << " topUser = \"" << h.topUser.c_str() 
        << "\", topMoves = \"" << topmoves1 
        << "\", topTime = \"" << h.topTime 
       << "\", topUser2 = \"" << h.topUser2.c_str() 
        << "\", topMoves2 = \"" << topmoves2
        << "\", topTime2 = \"" << h.topTime2
       << "\", topUser3 = \"" << h.topUser3.c_str() 
        << "\", topMoves3 = \"" << topmoves3
        << "\", topTime3 = \"" << h.topTime3
        << "\" WHERE bid = " << boardIdStr << ";";
    cout << st.str() << endl;
    s->execute(st.str().c_str());
    delete s;
    cout << "GOT TO B" << endl;
}
