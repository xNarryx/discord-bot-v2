#include "FileManager.h"
#include <fstream>
#include <iostream>

Guild gl;

void SetColorr(int color = 7) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void file_manager::load_api_keys(const std::string path)
{
    std::shared_lock lock(guilds_mutex);
    std::ifstream file(path);
    nlohmann::json j;
    file >> j;
    file.close();
    for (const auto& item : j["api_keys"]) {
        std::string name = item.value("name", "");
        std::string key = item.value("key", "");
        api_keys.insert({ name, key });
        std::cout << name << " " << key << "\n";
    }
    if (!api_keys.empty()) {
        std::cout << "Api_load path: " << path << " status: ";
        SetColorr(2);
        std::cout << "Succsesful\n";
        SetColorr();
    }
    else {
        std::cout << "Api_load status:";
        SetColorr(12);
        std::cout << "Fail\n";
        SetColorr();
    }
}

std::string file_manager::get_api_key(const std::string api)
{
    std::shared_lock lock(guilds_mutex);
    if (api_keys.contains(api)) {

        return std::string(api_keys[api]);
    }
    else {
        std::cout << "FAIL TO GET API\n";
        return std::string("");
    }
    
}

std::unordered_map<std::string, std::string> file_manager::get_all_api_keys()
{
    std::shared_lock lock(guilds_mutex);
    return std::unordered_map<std::string, std::string>(api_keys);
}

void file_manager::add_guild(const Guild& g)
{
    std::unique_lock lock(guilds_mutex);
    guilds[g.get_id()] = g;
}

std::unordered_map<dpp::snowflake, Guild>& file_manager::get_guilds()
{
  
    std::unique_lock lock(guilds_mutex);
    return guilds;
}

std::unordered_map<dpp::snowflake, Guild> file_manager::get_guilds_r() const
{
 
    std::shared_lock lock(guilds_mutex);
    return guilds;
}

Guild& file_manager::get_guild(const dpp::snowflake& guild_id)
{
    std::unique_lock lock(guilds_mutex);

    if (guilds.contains(guild_id)) {
        return guilds.at(guild_id);
    }

    
    gl.create_guild(guild_id);
    guilds[guild_id] = gl;

    return guilds[guild_id];
}

Guild& file_manager::get_guild_r(const dpp::snowflake& guild_id)
{
    std::shared_lock lock(guilds_mutex);

    if (guilds.contains(guild_id)) {
        return guilds.at(guild_id);
    }

}

bool file_manager::find_guild(dpp::snowflake& guild_id)
{
    std::shared_lock lock(guilds_mutex);
    return guilds.contains(guild_id);
}

bool file_manager::delete_guild_for_now(dpp::snowflake& guild_id)
{
    std::shared_lock lock(guilds_mutex);

    if (guilds.contains(guild_id)) {
        guilds.erase(guild_id);
        return true;
    }
    else {
        return false;
    }
}

bool file_manager::delete_guild(dpp::snowflake& guild_id, const std::string& folder_path)
{
    std::shared_lock lock(guilds_mutex);
    if (guilds.contains(guild_id)) {
        guilds.erase(guild_id);
        if (std::filesystem::remove(folder_path + std::to_string(guild_id) + ".json")) {
            std::cout << "Файл удалён\n";
            return true;
        }
        else {
            std::cout << "Файл не найден или не удалён\n";
            return false;
        }
        
    }
    return false;
}

bool file_manager::save_guilds(const std::string& folder_path)
{
    std::shared_lock lock(guilds_mutex);

    for (const auto& [gid, guild] : guilds)
    {
        std::string path = folder_path + std::to_string(gid) + ".json";
        std::ofstream file(path);

        if (!file.is_open()) {
            std::cout << "Cannot open file skipping: " << path << "\n";
        }

        file << guild.to_json().dump(0);
        file.close();
    }

    return true;
}

bool file_manager::load_guilds(const std::string& folder_path)
{
    std::unique_lock lock(guilds_mutex); 

    guilds.clear();

    try {
        bool loaded = true;
        for (const auto& entry : fs::directory_iterator(folder_path))
        {
            
            if (!entry.is_regular_file() || entry.path().extension() != ".json")
                continue;
            std::cout << "Loading path: " << entry.path();
            std::ifstream file(entry.path());
            if (!file.is_open()) continue;

            nlohmann::json j;
            dpp::snowflake gid = 0;
            try {
            file >> j;
            file.close();
            
                gid = j.value("guild_id", 0ULL);
            }
            catch (...) {
                loaded = false;
                file.close();
            }
            
            try {
                if (true) {
                    std::cout << " status: ";
                    guilds[gid] = Guild::from_json(j);
                    SetColorr(2);
                    std::cout << "Sucessfuly";
                    SetColorr();
                    std::cout << " Guild_id: " << gid << "\n";
                }
                else {
                    std::cout << " status: ";
                    SetColorr(4);
                    std::cout << "Failed\n";
                    SetColorr();
                }
            }
            catch (...) {
                std::cout << "Failed\n"; std::cout << "Failed\n"; std::cout << "Failed\n";
            }
            
        }
        if (loaded) {
            return true;
        }
        else {
            return false;
        }
    }
    catch (...) {
        SetColorr(4);
        std::cout << "failed loading path!\n";
        SetColorr();
        return false;
    }
}
