#include "../inc/Auth.hpp"
#include <string>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <sodium.h>
#include "mysql_driver.h"
using namespace std;

const char *DB_USER = "blockescape";
const char *DB_PASS = "horthownavlokum3";

void Auth::updateCompletedBoards(int index, const string &username){
    sql::Driver *driver = get_driver_instance();
    sql::Connection *con = 
        driver->connect("localhost", DB_USER, DB_PASS);
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
    delete con;
}
std::vector<int> Auth::getCompletedBoards(const std::string &username){
    sql::Driver *driver = get_driver_instance();
    sql::Connection *con = 
        driver->connect("localhost", DB_USER, DB_PASS);
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
    delete con;
    return ret;
}
bool Auth::userExists(const string &username){
    sql::Driver *driver = get_driver_instance();
    sql::Connection *con = 
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
        delete con;
        return true;
    }
    delete s;
    delete r;
    delete con;
    return false;
}
bool Auth::createUser(const string &username,
    const string &password, const string &salt){
    bool success=true;
    sql::Driver *driver = get_driver_instance();
    sql::Connection *con = 
        driver->connect("localhost", DB_USER, DB_PASS);
    con->setSchema("users");
    sql::Statement *s = con->createStatement();
    string insert("INSERT INTO users (username, password, salt, levels) VALUES (\"");
    insert += username;
    insert += "\", \"";
    insert += password;
    insert += "\", \"";
    insert += salt;
    insert += "\", \"";
    for(int i=0;i<3000;++i)
        insert.push_back('0');
    insert += "\");";
    try{
        s->execute(insert.c_str());
    }
    catch(sql::SQLException e){
        success = false;
    }
    delete s;
    delete con;
    return success;
}
bool Auth::Authorize(const string &username, const string &salt2,
    const string &password){
    sql::Driver *driver = get_driver_instance();
    sql::Connection *con = 
        driver->connect("localhost", DB_USER, DB_PASS);
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
    delete con;
    return finalHash == password;
}
string Auth::getSalt(const string &username){
    sql::Driver *driver = get_driver_instance();
    sql::Connection *con = 
        driver->connect("localhost", DB_USER, DB_PASS);
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
    delete con;
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