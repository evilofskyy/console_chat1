#define _CRT_SECURE_NO_WARNINGS

#include "Chat_Manager.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <cstring>

std::string ChatManager::hashToString(const uint* hash) {
    std::stringstream ss;
    for (int i = 0; i < SHA1HASHLENGTHUINTS; ++i) {
        ss << std::hex << std::setw(8) << std::setfill('0') << hash[i];
    }
    return ss.str();
}

void ChatManager::stringToHash(const std::string& str, uint* hash) {
    if (str.length() != SHA1HASHLENGTHUINTS * 8) {
        throw std::invalid_argument("Invalid hash string length");
    }
    for (int i = 0; i < SHA1HASHLENGTHUINTS; ++i) {
        std::string part = str.substr(i * 8, 8);
        hash[i] = std::stoul(part, nullptr, 16);
    }
}

ChatManager::ChatManager() {
    loadUsersFromFile();
    loadMessagesFromFile();
}

ChatManager::~ChatManager() {
    saveUsersToFile();
    saveMessagesToFile();
}

void ChatManager::loadUsersFromFile() {
    sqlite3* db;
    // Попытка открыть БД
    if (sqlite3_open("data/chat.db", &db) != SQLITE_OK) {
        std::cout << "ОШИБКА SQLITE (Users): Не удалось открыть data/chat.db! " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    // Создаем таблицу пользователей, если её нет
    const char* sql_create = "CREATE TABLE IF NOT EXISTS users (login TEXT PRIMARY KEY, hash TEXT, name TEXT);";
    sqlite3_exec(db, sql_create, 0, 0, 0);

    // Загружаем пользователей
    sqlite3_stmt* stmt;
    const char* sql_select = "SELECT login, hash, name FROM users;";
    if (sqlite3_prepare_v2(db, sql_select, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string login = (const char*)sqlite3_column_text(stmt, 0);
            std::string hashStr = (const char*)sqlite3_column_text(stmt, 1);
            std::string name = (const char*)sqlite3_column_text(stmt, 2);

            uint hash[SHA1HASHLENGTHUINTS];
            try {
                stringToHash(hashStr, hash);
                userTable.insert(login, hash, name);
            }
            catch (const std::exception& e) {
                std::cout << "ОШИБКА загрузки пользователя " << login << ": " << e.what() << std::endl;
            }
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void ChatManager::saveUsersToFile() const {
    sqlite3* db;
    if (sqlite3_open("data/chat.db", &db) != SQLITE_OK) {
        std::cout << "ОШИБКА SQLITE (Save Users): Не удалось открыть data/chat.db" << std::endl;
        return;
    }

    // Очищаем таблицу перед записью (перезапись)
    sqlite3_exec(db, "DELETE FROM users;", 0, 0, 0);

    sqlite3_stmt* stmt;
    const char* sql_insert = "INSERT INTO users (login, hash, name) VALUES (?, ?, ?);";
    sqlite3_prepare_v2(db, sql_insert, -1, &stmt, nullptr);

    auto allUsers = userTable.getAllOccupied();
    for (const auto& entry : allUsers) {
        sqlite3_bind_text(stmt, 1, entry.login.c_str(), -1, SQLITE_STATIC);
        std::string hashStr = hashToString(entry.hash);
        sqlite3_bind_text(stmt, 2, hashStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, entry.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void ChatManager::loadMessagesFromFile() {
    sqlite3* db;
    if (sqlite3_open("data/chat.db", &db) != SQLITE_OK) {
        std::cout << "ОШИБКА SQLITE (Messages): Не удалось открыть data/chat.db! " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    const char* sql_create = "CREATE TABLE IF NOT EXISTS messages (sender TEXT, recipient TEXT, text TEXT, time INTEGER);";
    sqlite3_exec(db, sql_create, 0, 0, 0);

    sqlite3_stmt* stmt;
    const char* sql_select = "SELECT sender, recipient, text, time FROM messages;";
    if (sqlite3_prepare_v2(db, sql_select, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string sender = (const char*)sqlite3_column_text(stmt, 0);
            std::string recipient = (const char*)sqlite3_column_text(stmt, 1);
            std::string text = (const char*)sqlite3_column_text(stmt, 2);
            std::time_t time = static_cast<std::time_t>(sqlite3_column_int64(stmt, 3));
            allMessages.push_back(Message(sender, recipient, text, time));
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void ChatManager::saveMessagesToFile() const {
    sqlite3* db;
    if (sqlite3_open("data/chat.db", &db) != SQLITE_OK) return;

    // Очищаем таблицу перед записью
    sqlite3_exec(db, "DELETE FROM messages;", 0, 0, 0);

    sqlite3_stmt* stmt;
    const char* sql_insert = "INSERT INTO messages (sender, recipient, text, time) VALUES (?, ?, ?, ?);";
    sqlite3_prepare_v2(db, sql_insert, -1, &stmt, nullptr);

    for (const auto& msg : allMessages) {
        sqlite3_bind_text(stmt, 1, msg.getSender().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, msg.getRecipient().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, msg.getText().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(msg.getTime()));
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

std::string ChatManager::formatTime(std::time_t time) const {
    char buffer[26];
    struct tm* timeInfo = std::localtime(&time);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeInfo);
    return std::string(buffer);
}

bool ChatManager::userExists(const std::string& login) const {
    return userTable.find(login) != nullptr;
}

void ChatManager::validateUserInput(const std::string& login, const std::string& password, const std::string& name) const {
    if (login.empty() || password.empty() || name.empty())
        throw std::invalid_argument("All fields must be filled");
    if (login.length() < 3)
        throw std::invalid_argument("Login must be at least 3 characters");
    if (password.length() < 4)
        throw std::invalid_argument("Password must be at least 4 characters");
}

std::vector<Message> ChatManager::getMessagesForUser(const std::string& userLogin, bool onlyPrivate) const {
    std::vector<Message> result;
    for (const auto& msg : allMessages) {
        bool isRecipient = (msg.getRecipient() == userLogin);
        bool isSender = (msg.getSender() == userLogin);
        if (onlyPrivate) {
            if ((isRecipient || isSender) && msg.getRecipient() != "ALL")
                result.push_back(msg);
        }
        else {
            if (msg.getRecipient() == "ALL")
                result.push_back(msg);
        }
    }
    return result;
}

void ChatManager::registerUser(const std::string& login, const std::string& password, const std::string& name) {
    std::lock_guard<std::mutex> lock(mtx);
    validateUserInput(login, password, name);
    if (userExists(login))
        throw std::runtime_error("User already exists");

    uint* hash = sha1(const_cast<char*>(password.c_str()), static_cast<uint>(password.length()));
    userTable.insert(login, hash, name);
    delete[] hash;
    saveUsersToFile();
}

bool ChatManager::verifyUser(const std::string& login, const std::string& password) const {
    std::lock_guard<std::mutex> lock(mtx);
    const UserData* data = userTable.find(login);
    if (!data) return false;

    uint* inputHash = sha1(const_cast<char*>(password.c_str()), static_cast<uint>(password.length()));
    bool match = true;
    for (int i = 0; i < SHA1HASHLENGTHUINTS; ++i) {
        if (inputHash[i] != data->hash[i]) {
            match = false;
            break;
        }
    }
    delete[] inputHash;
    return match;
}

bool ChatManager::doesUserExist(const std::string& login) const {
    std::lock_guard<std::mutex> lock(mtx);
    return userExists(login);
}

void ChatManager::sendPrivateMessage(const std::string& senderLogin, const std::string& recipientLogin, const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx);
    if (message.empty()) throw std::invalid_argument("Empty message");
    if (!userExists(recipientLogin)) throw std::runtime_error("Recipient not found");
    allMessages.push_back(Message(senderLogin, recipientLogin, message, std::time(nullptr)));
    saveMessagesToFile();
}

void ChatManager::sendPublicMessage(const std::string& senderLogin, const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "DEBUG: Sender=" << senderLogin << ", Msg=[" << message << "]" << std::endl;
    if (message.empty()) throw std::invalid_argument("Empty message");
    allMessages.push_back(Message(senderLogin, "ALL", message, std::time(nullptr)));
    saveMessagesToFile();
}

std::vector<Message> ChatManager::getPrivateMessagesRaw(const std::string& userLogin) const {
    std::lock_guard<std::mutex> lock(mtx);
    return getMessagesForUser(userLogin, true);
}

std::vector<Message> ChatManager::getPublicMessagesRaw(const std::string& userLogin) const {
    std::lock_guard<std::mutex> lock(mtx);
    return getMessagesForUser(userLogin, false);
}

std::vector<std::string> ChatManager::getAllUsersExcept(const std::string& userLogin) const {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::string> result;
    auto allUsers = userTable.getAllOccupied();
    for (const auto& entry : allUsers) {
        if (entry.login != userLogin)
            result.push_back(entry.login + " (" + entry.name + ")");
    }
    return result;
}

void ChatManager::clearUserMessages(const std::string& userLogin) {
    std::lock_guard<std::mutex> lock(mtx);
    allMessages.erase(std::remove_if(allMessages.begin(), allMessages.end(),
        [&userLogin](const Message& msg) {
            return msg.getSender() == userLogin || msg.getRecipient() == userLogin;
        }), allMessages.end());
    saveMessagesToFile();
}