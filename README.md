# Сетевой чат (Client-Server) на C++ с хранением в SQLite

Реализация клиент-серверного чата с использованием TCP-сокетов и базы данных SQLite.

## Подготовка (Очень важно!)
Для работы сервера вам понадобится библиотека SQLite. 
1. Скачайте файлы `sqlite3.h` и `sqlite3.c` с официального сайта SQLite: https://www.sqlite.org/download.html (выберите раздел "Source Code", скачайте архив sqlite-amalgamation).
2. Распакуйте и положите эти 2 файла в одну папку с исходным кодом (рядом с `chat_server.cpp`).

## Компиляция в Windows (Visual Studio Developer Command Prompt)
1. Открыть **Developer Command Prompt for VS 2022**.
2. Перейти в папку с проектом (команда `cd`).
3. Собрать **Сервер** (включая файл SQLite):
```cmd
cl /EHsc /std:c++17 Chat_Manager.cpp chat_server.cpp sqlite3.c sha1.cpp HashTable.cpp Message.cpp User.cpp /Fe:chat_server.exe /link ws2_32.lib