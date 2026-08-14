#define _CRT_SECURE_NO_WARNINGS
#include "Message.h"

Message::Message(const std::string& _sender, const std::string& _recipient, const std::string& _text, std::time_t _time)
    : sender(_sender), recipient(_recipient), text(_text), time(_time) {
}

std::string Message::getSender() const { return sender; }
std::string Message::getRecipient() const { return recipient; }
std::string Message::getText() const { return text; }
std::time_t Message::getTime() const { return time; }