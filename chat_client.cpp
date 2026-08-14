#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <sstream>
#include <cstdint>

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

const char DELIMITER = '|';
const int PORT = 8888;
const std::string SERVER_IP = "127.0.0.1";

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

int getIntInput(const std::string& prompt) {
    int value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Error: enter a number.\n";
        }
        else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
    }
}

void showMainMenu() {
    std::cout << "\n=== Main Menu ===\n";
    std::cout << "1. Register\n";
    std::cout << "2. Login\n";
    std::cout << "3. Exit\n";
}

void showUserMenu() {
    std::cout << "\n=== User Menu ===\n";
    std::cout << "1. Send private message\n";
    std::cout << "2. Send public message\n";
    std::cout << "3. View private messages\n";
    std::cout << "4. View public messages\n";
    std::cout << "5. List users\n";
    std::cout << "6. Clear my messages\n";
    std::cout << "7. Logout\n";
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;
#endif

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return 1;

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP.c_str());

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        std::cout << "Cannot connect to server. Make sure server is running.\n";
        return 1;
    }

    bool running = true;
    bool logged_in = false;
    std::string currentUserLogin;

    while (running) {
        if (!logged_in) {
            showMainMenu();
            int choice = getIntInput("Choose: ");

            if (choice == 1) {
                std::string login, password, name;
                std::cout << "Login: "; std::getline(std::cin, login);
                std::cout << "Password: "; std::getline(std::cin, password);
                std::cout << "Name: "; std::getline(std::cin, name);
                std::string req = "REGISTER" + std::string(1, DELIMITER) + login + DELIMITER + password + DELIMITER + name;
                send_all(sock, req);
                std::string resp = recv_all(sock);
                if (resp == "OK") std::cout << "Registration successful!\n";
                else if (resp.empty()) std::cout << "Ошибка: сервер не ответил. Проверьте брандмауэр.\n";
                else std::cout << "Error: " << resp << "\n";
            }
            else if (choice == 2) {
                std::string login, password;
                std::cout << "Login: "; std::getline(std::cin, login);
                std::cout << "Password: "; std::getline(std::cin, password);
                std::string req = "LOGIN" + std::string(1, DELIMITER) + login + DELIMITER + password;
                send_all(sock, req);
                std::string resp = recv_all(sock);
                if (resp == "OK") {
                    logged_in = true;
                    currentUserLogin = login;
                    std::cout << "Welcome, " << login << "!\n";
                }
                else if (resp.empty()) std::cout << "Ошибка: сервер не ответил. Проверьте брандмауэр.\n";
                else std::cout << "Error: " << resp << "\n";
            }
            else if (choice == 3) {
                running = false;
            }
        }
        else {
            showUserMenu();
            int choice = getIntInput("Choose: ");

            if (choice == 1) {
                std::string recipient, message;
                std::cout << "Recipient: "; std::getline(std::cin, recipient);
                std::cout << "Message: "; std::getline(std::cin, message);
                std::string req = "SEND_PRIVATE" + std::string(1, DELIMITER) + currentUserLogin + DELIMITER + recipient + DELIMITER + message;
                send_all(sock, req);
                std::string resp = recv_all(sock);
                if (resp == "OK") std::cout << "Message sent.\n";
                else if (resp.empty()) std::cout << "Ошибка: сервер не ответил. Проверьте брандмауэр.\n";
                else std::cout << "Error: " << resp << "\n";
            }
            else if (choice == 2) {
                std::string message;
                std::cout << "Message: "; std::getline(std::cin, message);
                std::string req = "SEND_PUBLIC" + std::string(1, DELIMITER) + currentUserLogin + DELIMITER + message;
                send_all(sock, req);
                std::string resp = recv_all(sock);
                if (resp == "OK") std::cout << "Message sent.\n";
                else if (resp.empty()) std::cout << "Ошибка: сервер не ответил. Проверьте брандмауэр.\n";
                else std::cout << "Error: " << resp << "\n";
            }
            else if (choice == 3) {
                std::string req = "GET_PRIVATE" + std::string(1, DELIMITER) + currentUserLogin;
                send_all(sock, req);
                std::string resp = recv_all(sock);
                auto parts = split(resp, DELIMITER);
                if (parts[0] == "MESSAGES") {
                    if (parts.size() == 1) std::cout << "No private messages.\n";
                    else for (size_t i = 1; i < parts.size(); ++i) std::cout << parts[i] << "\n";
                }
                else if (resp.empty()) std::cout << "Ошибка: сервер не ответил.\n";
                else std::cout << "Error: " << resp << "\n";
            }
            else if (choice == 4) {
                std::string req = "GET_PUBLIC" + std::string(1, DELIMITER) + currentUserLogin;
                send_all(sock, req);
                std::string resp = recv_all(sock);
                auto parts = split(resp, DELIMITER);
                if (parts[0] == "MESSAGES") {
                    if (parts.size() == 1) std::cout << "No public messages.\n";
                    else for (size_t i = 1; i < parts.size(); ++i) std::cout << parts[i] << "\n";
                }
                else if (resp.empty()) std::cout << "Ошибка: сервер не ответил.\n";
                else std::cout << "Error: " << resp << "\n";
            }
            else if (choice == 5) {
                std::string req = "GET_USERS" + std::string(1, DELIMITER) + currentUserLogin;
                send_all(sock, req);
                std::string resp = recv_all(sock);
                auto parts = split(resp, DELIMITER);
                if (parts[0] == "USERS") {
                    if (parts.size() == 1) std::cout << "No other users.\n";
                    else for (size_t i = 1; i < parts.size(); ++i) std::cout << parts[i] << "\n";
                }
                else if (resp.empty()) std::cout << "Ошибка: сервер не ответил.\n";
                else std::cout << "Error: " << resp << "\n";
            }
            else if (choice == 6) {
                std::string req = "CLEAR_MESSAGES" + std::string(1, DELIMITER) + currentUserLogin;
                send_all(sock, req);
                std::string resp = recv_all(sock);
                if (resp == "OK") std::cout << "Messages cleared.\n";
                else if (resp.empty()) std::cout << "Ошибка: сервер не ответил.\n";
                else std::cout << "Error: " << resp << "\n";
            }
            else if (choice == 7) {
                logged_in = false;
                currentUserLogin = "";
                std::cout << "Logged out.\n";
            }
        }
    }

    closesocket(sock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}