#include "store.hpp"

void Store::set(const std::string &key, const std::string &value) {

    size_t idx = get_shard_index(key);
    
    std::lock_guard<std::mutex> lock(shards[idx].mtx);
    
    shards[idx].kv[key] = value;
}

std::string Store::get(const std::string &key) {
    size_t idx = get_shard_index(key);
    
    std::lock_guard<std::mutex> lock(shards[idx].mtx);
    
    auto it = shards[idx].kv.find(key);
    if(it != shards[idx].kv.end()) {
        return it->second;
    }
    return "NOT_FOUND";
}

void Store::del(const std::string &key) {
    size_t idx = get_shard_index(key);
    
    std::lock_guard<std::mutex> lock(shards[idx].mtx);
    
    shards[idx].kv.erase(key);
}