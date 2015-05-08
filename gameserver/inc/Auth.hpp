#ifndef AUTH_HPP_
#define AUTH_HPP_
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <string>
#include <vector>
class Auth {
public:
    std::vector<int> getCompletedBoards(const std::string &username);
    void updateCompletedBoards(int index, const std::string &username);
    bool userExists(const std::string &username);
    bool createUser(const std::string &username,
        const std::string &password, const std::string &salt);
    bool Authorize(const std::string &username, const std::string &salt2,
        const std::string &password);
    std::string getSalt(const std::string &username);
    std::string genSalt();
    static Auth *getInstance();
    ~Auth();
private:
    Auth();
    static Auth *instance;
    sql::Driver *driver;
    sql::Connection *con;
};
#endif
