#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <iostream>
#include "guild.h"
#include "User.h"
#include <unordered_map>
#include <filesystem>
#include <shared_mutex>

namespace fs = std::filesystem;

class file_manager {
private:
    std::unordered_map<dpp::snowflake, std::shared_ptr<Guild>> guilds;
    mutable std::shared_mutex guilds_mutex;
    mutable std::shared_mutex api_keys_mutex;
    std::unordered_map <std::string, std::string> api_keys;

public:
    void load_api_keys(const std::string path);
    std::string get_api_key(const std::string api);
    std::unordered_map <std::string, std::string> get_all_api_keys();


    std::shared_ptr<Guild> get_guild(const dpp::snowflake id);
    std::vector<std::shared_ptr<Guild>> get_guilds();
    std::shared_ptr<Guild> find_guild(const dpp::snowflake id)
    {
        std::shared_lock lock(guilds_mutex);
        auto it = guilds.find(id);
        if (it != guilds.end()) {
            return it->second;
        }
        return nullptr;
    }
    void add_guild(std::shared_ptr<Guild> g);
    template<typename Func>
    auto modify_guild(const dpp::snowflake& id, Func&& func) {
        auto g = get_guild(id); 
        return g->modify(std::forward<Func>(func));
    }

    template<typename Func>
    auto read_guild(const dpp::snowflake& id, Func&& func)
        -> std::optional<std::invoke_result_t<Func, const Guild&>>
    {
        auto g = find_guild(id);
        if (!g) {
            return std::nullopt;
        }
        return g->read(std::forward<Func>(func));
    }
	

    bool delete_guild_for_now(dpp::snowflake& guild_id);
    bool delete_guild(dpp::snowflake& guild_id, const std::string& folder_path);
    bool save_guilds(const std::string& folder_path);
    bool load_guilds(const std::string& folder_path);
};
