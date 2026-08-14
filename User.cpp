#define _CRT_SECURE_NO_WARNINGS
#include "User.h"

User::User(const std::string& _login, const std::string& _password, const std::string& _name)
    : login(_login), password(_password), name(_name) {
}

std::string User::getLogin() const { return login; }
std::string User::getName() const { return name; }
std::string User::getPassword() const { return password; }
bool User::checkPassword(const std::string& _password) const { return password == _password; }