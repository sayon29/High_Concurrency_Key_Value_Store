#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <array>
#include <functional>

constexpr size_t NUM_SHARDS = 16; 

class Store {
private:
    // Group the lock and the map together for cache locality
    struct Shard {
        std::mutex mtx;
        std::unordered_map<std::string, std::string> kv;
    };

    //array of independent shards
    std::array<Shard, NUM_SHARDS> shards;
    
    // Hash function to route keys to shards
    std::hash<std::string> hasher;

    size_t get_shard_index(const std::string& key) const {
        return hasher(key) % NUM_SHARDS;
    }

public:
    void set(const std::string &key, const std::string &value);
    std::string get(const std::string &key);
    void del(const std::string &key);
};