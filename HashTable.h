#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include "sha1.h"

struct UserData {
    std::string login;
    uint hash[SHA1HASHLENGTHUINTS];
    std::string name;
    bool occupied = false;
    bool deleted = false;
};

class HashTable {
private:
    static constexpr double GOLDEN_RATIO = 0.6180339887498949;
    static constexpr int INITIAL_SIZE = 101;
    static constexpr double LOAD_FACTOR = 0.75;

    int table_size;
    int count;
    std::vector<UserData> table;

    int hash_func(const std::string& key) const;
    bool is_prime(int n) const;
    int next_prime(int n) const;
    void rehash();

public:
    HashTable();
    ~HashTable() = default;

    void insert(const std::string& login, const uint* hash, const std::string& name);
    const UserData* find(const std::string& login) const;
    bool remove(const std::string& login);
    std::vector<UserData> getAllOccupied() const;
    void clear();
    int size() const { return count; }
};