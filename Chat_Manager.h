#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <ctime>
#include "User.h"
#include "Message.h"
#include "HashTable.h"
#include "sqlite3.h" // Добавляем SQLite

struct TextFormat {};
struct JsonFormat {};

class ChatManager {
private:
    HashTable userTable;
    std::vector<Message> allMessages;
    mutable std::mutex mtx;

    void loadUsersFromFile();
    void saveUsersToFile() const;
    void loadMessagesFromFile();
    void saveMessagesToFile() const;
    std::string getUsersFilename() const { return "data/users.txt"; }
    std::string getMessagesFilename() const { return "data/messages.txt"; }

    bool userExists(const std::string& login) const;
    void validateUserInput(const std::string& login, const std::string& password, const std::string& name) const;
    std::vector<Message> getMessagesForUser(const std::string& userLogin, bool onlyPrivate) const;
    std::string formatTime(std::time_t time) const;

    static std::string hashToString(const uint* hash);
    static void stringToHash(const std::string& str, uint* hash);

public:
    ChatManager();
    ~ChatManager();

    void registerUser(const std::string& login, const std::string& password, const std::string& name);
    bool verifyUser(const std::string& login, const std::string& password) const;
    bool doesUserExist(const std::string& login) const;

    void sendPrivateMessage(const std::string& senderLogin, const std::string& recipientLogin, const std::string& message);
    void sendPublicMessage(const std::string& senderLogin, const std::string& message);

    std::vector<Message> getPrivateMessagesRaw(const std::string& userLogin) const;
    std::vector<Message> getPublicMessagesRaw(const std::string& userLogin) const;

    std::vector<std::string> getAllUsersExcept(const std::string& userLogin) const;
    void clearUserMessages(const std::string& userLogin);
};

// Шаблоны форматирования
template<typename Format>
std::string formatMessage(const Message& msg, std::time_t currentTime, const std::string& currentUserLogin = "");

template<>
inline std::string formatMessage<TextFormat>(const Message& msg, std::time_t /*currentTime*/, const std::string& currentUserLogin) {
    std::stringstream ss;
    if (msg.getRecipient() == "ALL") {
        ss << "[Public] " << msg.getSender() << ": " << msg.getText();
    }
    else {
        if (currentUserLogin.empty() || msg.getSender() == currentUserLogin) {
            ss << "[To " << msg.getRecipient() << "] " << msg.getText();
        }
        else {
            ss << "[From " << msg.getSender() << "] " << msg.getText();
        }
    }
    return ss.str();
}

template<>
inline std::string formatMessage<JsonFormat>(const Message& msg, std::time_t currentTime, const std::string& currentUserLogin) {
    char timeBuffer[26];
#ifdef _WIN32
    ctime_s(timeBuffer, sizeof(timeBuffer), &currentTime);
#else
    ctime_r(&currentTime, timeBuffer);
#endif
    std::string timeStr(timeBuffer);
    if (!timeStr.empty() && timeStr.back() == '\n') timeStr.pop_back();

    std::stringstream ss;
    ss << "{"
        << "\"sender\":\"" << msg.getSender() << "\","
        << "\"recipient\":\"" << msg.getRecipient() << "\","
        << "\"text\":\"" << msg.getText() << "\","
        << "\"time\":" << msg.getTime() << ","
        << "\"time_str\":\"" << timeStr << "\"";
    if (!currentUserLogin.empty()) {
        ss << ",\"is_own\":" << (msg.getSender() == currentUserLogin ? "true" : "false");
    }
    ss << "}";
    return ss.str();
}