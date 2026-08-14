#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

#include <iostream>
#include <thread>
#include <cstring>
#include <string>
#include <sstream>
#include <cstdint>
#include <vector>

#include "Chat_Manager.h"

const char DELIMITER = '|';
const int PORT = 8888;

bool send_all(SOCKET sock, const std::string& data) {
    uint32_t len = htonl((uint32_t)data.size());
    if (send(sock, (const char*)&len, 4, 0) != 4) return false;
    size_t sent = 0;
    while (sent < data.size()) {
        int res = send(sock, data.c_str() + sent, (int)(data.size() - sent), 0);
        if (res <= 0) return false;
        sent += res;
    }
    return true;
}

std::string recv_all(SOCKET sock) {
    uint32_t len_net;
    if (recv(sock, (char*)&len_net, 4, 0) != 4) return "";
    uint32_t len = ntohl(len_net);
    std::string buf;
    buf.resize(len);
    size_t recvd = 0;
    while (recvd < len) {
        int res = recv(sock, (char*)buf.data() + recvd, (int)(len - recvd), 0);
        if (res <= 0) return "";
        recvd += res;
    }
    return buf;
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string handle_request(const std::string& request, ChatManager& manager) {
    // Лог для отладки
    std::cout << ">>> СЕРВЕР ПОЛУЧИЛ: [" << request << "]" << std::endl;

    // Разбиваем строку
    auto parts = split(request, DELIMITER);
    std::cout << ">>> КОЛИЧЕСТВО ЧАСТЕЙ: " << parts.size() << std::endl;
    for (const auto& part : parts) {
        std::cout << "   ЧАСТЬ: [" << part << "]" << std::endl;
    }

    if (parts.empty()) return "ERROR" + std::string(1, DELIMITER) + "Empty command";

    std::string cmd = parts[0];

    try {
        if (cmd == "REGISTER") {
            manager.registerUser(parts[1], parts[2], parts[3]);
            return "OK";
        }
        else if (cmd == "LOGIN") {
            bool success = manager.verifyUser(parts[1], parts[2]);
            std::cout << "Попытка входа для пользователя [" << parts[1] << "]: "
                << (success ? "УСПЕШНО" : "НЕВЕРНЫЙ ПАРОЛЬ ИЛИ НЕТ ПОЛЬЗОВАТЕЛЯ") << std::endl;
            return success ? "OK" : "ERROR" + std::string(1, DELIMITER) + "Invalid login or password";
        }
        else if (cmd == "SEND_PRIVATE") {
            manager.sendPrivateMessage(parts[1], parts[2], parts[3]);
            return "OK";
        }
        else if (cmd == "SEND_PUBLIC") {
            manager.sendPublicMessage(parts[1], parts[2]);
            return "OK";
        }
        else if (cmd == "GET_PRIVATE") {
            auto msgs = manager.getPrivateMessagesRaw(parts[1]);
            std::string resp = "MESSAGES";
            for (const auto& msg : msgs) {
                resp += DELIMITER + formatMessage<TextFormat>(msg, msg.getTime(), parts[1]);
            }
            return resp;
        }
        else if (cmd == "GET_PUBLIC") {
            auto msgs = manager.getPublicMessagesRaw(parts[1]);
            std::string resp = "MESSAGES";
            for (const auto& msg : msgs) {
                resp += DELIMITER + formatMessage<TextFormat>(msg, msg.getTime(), parts[1]);
            }
            return resp;
        }
        else if (cmd == "GET_USERS") {
            auto users = manager.getAllUsersExcept(parts[1]);
            std::string resp = "USERS";
            for (const auto& u : users) {
                resp += DELIMITER + u;
            }
            return resp;
        }
        else if (cmd == "CLEAR_MESSAGES") {
            manager.clearUserMessages(parts[1]);
            return "OK";
        }

        // Если ни одна команда не подошла
        return "ERROR" + std::string(1, DELIMITER) + "Unknown command";
    }
    catch (const std::exception& e) {
        return std::string("ERROR") + DELIMITER + e.what();
    }
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;
#endif

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) return 1;

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) return 1;
    if (listen(server_fd, 5) == SOCKET_ERROR) return 1;

    std::cout << "Chat Server started on port " << PORT << std::endl;

    ChatManager manager;

    while (true) {
        sockaddr_in client_addr;
#ifdef _WIN32
        int addr_len = sizeof(client_addr);
#else
        socklen_t addr_len = sizeof(client_addr);
#endif
        SOCKET client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock == INVALID_SOCKET) continue;

        // ЗАПУСК ПОТОКА ДЛЯ КАЖДОГО КЛИЕНТА
        std::thread([client_sock, &manager]() {
            // Бесконечный цикл: держим соединение открытым, пока клиент не закроет его
            while (true) {
                std::string request = recv_all(client_sock);

                // Если клиент закрыл соединение или произошла ошибка - выходим из цикла
                if (request.empty()) {
                    std::cout << "!!! Клиент разорвал соединение !!!" << std::endl;
                    break;
                }

                // Обрабатываем запрос и отправляем ответ
                std::string response = handle_request(request, manager);
                if (!send_all(client_sock, response)) {
                    std::cout << "!!! Ошибка отправки клиенту, разрываем соединение !!!" << std::endl;
                    break;
                }
            }

            // Корректное закрытие сокета после выхода из цикла
#ifdef _WIN32
            shutdown(client_sock, SD_SEND);
            Sleep(100); // Даем время последним пакетам уйти
#else
            shutdown(client_sock, SHUT_WR);
            usleep(100000);
#endif
            closesocket(client_sock);
            }).detach(); // Отсоединяем поток, чтобы он работал в фоне
    }

    closesocket(server_fd);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}