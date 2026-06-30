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
    std::unordered_map<dpp::snowflake, Guild> guilds;
    mutable std::shared_mutex guilds_mutex;
    std::unordered_map <std::string, std::string> api_keys;

public:
    void load_api_keys(const std::string path);
    std::string get_api_key(const std::string api);
    std::unordered_map <std::string, std::string> get_all_api_keys();
    void add_guild(const Guild& g);


    std::unordered_map<dpp::snowflake, Guild>& get_guilds();
    std::unordered_map<dpp::snowflake, Guild> get_guilds_r() const;
    Guild& get_guild(const dpp::snowflake& guild_id);
    Guild& get_guild_r(const dpp::snowflake& guild_id);
    bool find_guild(dpp::snowflake& guild_id);

    bool delete_guild_for_now(dpp::snowflake& guild_id);
    bool delete_guild(dpp::snowflake& guild_id, const std::string& folder_path);
    bool save_guilds(const std::string& folder_path);
    bool load_guilds(const std::string& folder_path);
};
