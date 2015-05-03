#include "../inc/Auth.hpp"
#include <string>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <sodium.h>
#include "mysql_driver.h"
using namespace std;
bool Auth::createUser(const string &username,
    const string &password, const string &salt){
    bool success=true;
    sql::Driver *driver = get_driver_instance();
    sql::Connection *con = 
        driver->connect("localhost", "root", "%%horthownav%%lokum3%%");
    con->setSchema("users");
    sql::Statement *s = con->createStatement();
    string insert("INSERT INTO users VALUES (\"");
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
        driver->connect("localhost", "root", "%%horthownav%%lokum3%%");
    con->setSchema("users");
    sql::Statement *s = con->createStatement();
    string select("SELECT password FROM users WHERE username = \"");
    select += username;
    select += "\";";
    sql::ResultSet *r = s->executeQuery(select.c_str());
    string dbpassword = salt2;
    if(r->next()){
        dbpassword+=r->getString("password");
    }
    unsigned char out[crypto_hash_sha256_BYTES];
    unsigned char *in = new unsigned char [dbpassword.size()];
    crypto_hash_sha256(out, in, dbpassword.size());
    delete [] in;
    string comp;
    int i = 0;
    while(out[i])
        comp.push_back(char(out[i++]));
    delete s;
    delete r;
    delete con;
    return comp == password;
}
string Auth::getSalt(const string &username){
    sql::Driver *driver = get_driver_instance();
    sql::Connection *con = 
        driver->connect("localhost", "root", "%%horthownav%%lokum3%%");
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
    string s;
    while(s.size() < 32){
        char c = randombytes_random() & 0xFF;
        if((c >= 'a' && c <= 'z')||(c>= 'A' && c <= 'Z'))
            s.push_back(c);
    }
    return s;
}
