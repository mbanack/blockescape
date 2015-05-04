#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <iostream>
using namespace std;
int main(){
    sql::Driver *driver = get_driver_instance();
    sql::Connection *con = 
        driver->connect("localhost", "blockescape", "%%horthownav%%lokum3%%");
    con->setSchema("users");
    sql::Statement *s = con->createStatement();
    string select("SELECT * FROM users;");
    sql::ResultSet *r = s->executeQuery(select.c_str());
    while(r->next()){
        cout 
            << r->getString("username") << "."
            << r->getString("password") << "."
            << r->getString("salt") << "." << endl;
    }
}
