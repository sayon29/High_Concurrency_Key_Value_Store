#include "store.hpp"

void Store::set(const std::string &key, const std::string &value){
    std::lock_guard<std::mutex> lock(mtx);
    kv[key] = value;
}

std::string Store::get(const std::string &key){
    std::lock_guard<std::mutex> lock(mtx);
    auto it = kv.find(key);
    if(it != kv.end()){
        return it->second;
    }
    return "NOT_FOUND";
}

void Store::del(const std::string &key){
    std::lock_guard<std::mutex> lock(mtx);
    kv.erase(key);
}