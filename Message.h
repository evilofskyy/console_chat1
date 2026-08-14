#pragma once
#include <string>
#include <ctime>

class Message {
private:
    std::string sender;
    std::string recipient;
    std::string text;
    std::time_t time;
public:
    Message(const std::string& _sender, const std::string& _recipient, const std::string& _text, std::time_t _time);
    std::string getSender() const;
    std::string getRecipient() const;
    std::string getText() const;
    std::time_t getTime() const;
};