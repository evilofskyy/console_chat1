#define _CRT_SECURE_NO_WARNINGS
#include "HashTable.h"
#include <algorithm>
#include <cmath>

HashTable::HashTable() : table_size(INITIAL_SIZE), count(0) {
    table.resize(table_size);
}

int HashTable::hash_func(const std::string& key) const {
    unsigned long long h = 0;
    for (char c : key) {
        h = h * 31 + static_cast<unsigned char>(c);
    }
    double A = GOLDEN_RATIO;
    double frac = h * A - std::floor(h * A);
    return static_cast<int>(std::floor(table_size * frac));
}

bool HashTable::is_prime(int n) const {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0) return false;
    for (int d = 3; d * d <= n; d += 2)
        if (n % d == 0) return false;
    return true;
}

int HashTable::next_prime(int n) const {
    while (!is_prime(n)) ++n;
    return n;
}

void HashTable::rehash() {
    int old_size = table_size;
    std::vector<UserData> old_table = std::move(table);

    table_size = next_prime(old_size * 2);
    table.clear();
    table.resize(table_size);
    count = 0;

    for (const auto& entry : old_table) {
        if (entry.occupied && !entry.deleted) {
            insert(entry.login, entry.hash, entry.name);
        }
    }
}

void HashTable::insert(const std::string& login, const uint* hash, const std::string& name) {
    if ((double)(count + 1) / table_size > LOAD_FACTOR) {
        rehash();
    }

    int base = hash_func(login);
    int i = 0;
    int first_deleted = -1;

    while (i < table_size) {
        int idx = (base + i * i) % table_size;
        UserData& cell = table[idx];

        if (!cell.occupied) {
            if (first_deleted != -1) {
                idx = first_deleted;
                cell = table[idx];
            }
            cell.login = login;
            std::memcpy(cell.hash, hash, sizeof(uint) * SHA1HASHLENGTHUINTS);
            cell.name = name;
            cell.occupied = true;
            cell.deleted = false;
            ++count;
            return;
        }
        else if (cell.occupied && cell.deleted && first_deleted == -1) {
            first_deleted = idx;
        }
        else if (cell.occupied && !cell.deleted && cell.login == login) {
            std::memcpy(cell.hash, hash, sizeof(uint) * SHA1HASHLENGTHUINTS);
            cell.name = name;
            return;
        }
        ++i;
    }

    throw std::runtime_error("HashTable is full");
}

const UserData* HashTable::find(const std::string& login) const {
    int base = hash_func(login);
    int i = 0;
    while (i < table_size) {
        int idx = (base + i * i) % table_size;
        const UserData& cell = table[idx];
        if (!cell.occupied) {
            return nullptr;
        }
        if (cell.occupied && !cell.deleted && cell.login == login) {
            return &cell;
        }
        ++i;
    }
    return nullptr;
}

bool HashTable::remove(const std::string& login) {
    int base = hash_func(login);
    int i = 0;
    while (i < table_size) {
        int idx = (base + i * i) % table_size;
        UserData& cell = table[idx];
        if (!cell.occupied) return false;
        if (cell.occupied && !cell.deleted && cell.login == login) {
            cell.deleted = true;
            --count;
            return true;
        }
        ++i;
    }
    return false;
}

std::vector<UserData> HashTable::getAllOccupied() const {
    std::vector<UserData> result;
    for (const auto& cell : table) {
        if (cell.occupied && !cell.deleted) {
            result.push_back(cell);
        }
    }
    return result;
}

void HashTable::clear() {
    table.clear();
    table.resize(table_size);
    count = 0;
}