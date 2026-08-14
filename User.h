#pragma once
#include <string>

class User {
private:
    std::string login;
    std::string password;
    std::string name;
public:
    User(const std::string& _login, const std::string& _password, const std::string& _name);
    std::string getLogin() const;
    std::string getName() const;
    std::string getPassword() const;
    bool checkPassword(const std::string& _password) const;
};