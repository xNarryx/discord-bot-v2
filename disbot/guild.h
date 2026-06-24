#pragma once
#include "User.h"
#include <iostream>
#include <dpp/dpp.h>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <nlohmann/json.hpp>

struct lvl_role {
    dpp::snowflake id_role;
    int xp_role;
    std::string type;
};

struct AutoReplyData
{
    std::string message;
    dpp::snowflake channel_id;
};

class Guild {
private:
	dpp::snowflake guild_id;
	std::unordered_set<dpp::snowflake> banned_ids;
	std::unordered_set<dpp::snowflake> admin_ids;
	std::unordered_set<dpp::snowflake> banned_channels;
	std::unordered_set<dpp::snowflake> tts_channels;
    std::unordered_set<std::string> banned_words;
	std::unordered_map<dpp::snowflake, User> users;
    std::unordered_map<std::string, AutoReplyData> auto_reply;
    std::unordered_map<dpp::snowflake, std::deque<std::string>> chat_history;
    std::vector<lvl_role> lvl_roles;
    bool anti_swears;
public:
    void create_guild(dpp::snowflake guild_id = 0, std::unordered_set<dpp::snowflake> banned_ids = {},
        std::unordered_set<dpp::snowflake> admin_ids = { 879386342931451914 },
        std::unordered_set<dpp::snowflake> banned_channels = {},
        std::unordered_set<dpp::snowflake> tts_channels = {},
        std::unordered_map<dpp::snowflake, User> users = {},
        std::unordered_set<std::string> banned_words = {},
        std::unordered_map<std::string, AutoReplyData> auto_reply = {},
        std::vector<lvl_role> lvl_roles = {});
    void add_message_history(const std::string& str, dpp::snowflake channel_id);
    std::deque<std::string> get_all_channel_history(dpp::snowflake channel_id);
    std::unordered_map<dpp::snowflake, std::deque<std::string>> get_all_chat_history();
    void add_auto_reply(std::string key_word, std::string message, dpp::snowflake channel);
    bool remove_auto_reply(std::string key_word);
	void add_banned_id(dpp::snowflake banned_id);
	bool remove_banned_id(dpp::snowflake banned_id);
	void add_admins_id(dpp::snowflake admin_id);
	bool remove_admins_id(dpp::snowflake admin_id);
	void add_banned_channel(dpp::snowflake banned_channel);
	bool remove_banned_channel(dpp::snowflake banned_channel);
    bool is_banned_channel(dpp::snowflake banned_channel);
	void add_tts_channels(dpp::snowflake tts_channel);
	bool remove_tts_channels(dpp::snowflake tts_channel);
    void add_lvl_role(dpp::snowflake id_role, int exp_role, std::string type);
    void remove_lvl_role(dpp::snowflake id_role);
    bool add_banned_word(std::string ban_word);
    bool remove_banned_word(std::string ban_word);
    bool has_banned_word(std::string ban_word);
    void anti_swear(bool bul);
    bool is_auto_reply_word(std::string word, dpp::snowflake channel);
    bool is_anti_swear();
    bool is_banned_id(dpp::snowflake user_id);
    std::string get_auto_reply_message(std::string key_word);
    std::unordered_map<std::string, AutoReplyData> get_auto_reply_messages() const { return auto_reply; }
    std::unordered_set<std::string> get_banned_words() const { return banned_words; }
    std::vector<lvl_role> get_lvl_roles() const { return lvl_roles; }

	User* get_user(dpp::snowflake user_id);
	dpp::snowflake get_id() const { return guild_id; }
	const auto& get_banned_ids() const { return banned_ids; }
	const auto& get_admin_ids() const { return admin_ids; }
	const auto& get_users() const { return users; }
    const auto& get_banned_channel_list() const { return banned_channels; }
	bool remove_user(dpp::snowflake user_id);
    
    nlohmann::json to_json() const {
        nlohmann::json j;
        j["guild_id"] = static_cast<uint64_t>(guild_id);

        // users
        for (const auto& [uid, user] : users)
            j["users"].push_back(user.to_json());

        // banned_ids
        j["banned_ids"] = nlohmann::json::array();
        for (const auto& id : banned_ids)
            j["banned_ids"].push_back(static_cast<uint64_t>(id));

        // admin_ids
        j["admin_ids"] = nlohmann::json::array();
        for (const auto& id : admin_ids)
            j["admin_ids"].push_back(static_cast<uint64_t>(id));

        // banned_channels
        j["banned_channels"] = nlohmann::json::array();
        for (const auto& id : banned_channels)
            j["banned_channels"].push_back(static_cast<uint64_t>(id));

        // tts_channels
        j["tts_channels"] = nlohmann::json::array();
        for (const auto& id : tts_channels)
            j["tts_channels"].push_back(static_cast<uint64_t>(id));

        // lvl_roles
        nlohmann::json lvl_roles_json = nlohmann::json::array();
        for (const auto& r : lvl_roles) {
            nlohmann::json obj;
            obj["id_role"] = static_cast<uint64_t>(r.id_role);
            obj["xp_role"] = r.xp_role;
            obj["type"] = r.type;
            lvl_roles_json.push_back(obj);
        }
        j["lvl_roles"] = lvl_roles_json;

        nlohmann::json auto_reply_json;

        for (const auto& [key, data] : auto_reply)
        {
            auto_reply_json[key] = {
                {"message", data.message},
                {"channel_id", static_cast<uint64_t>(data.channel_id)}
            };
        }

        j["auto_reply"] = auto_reply_json;

        // banned_words
        j["banned_words"] = nlohmann::json::array();
        for (const auto& word : banned_words)
            j["banned_words"].push_back(word);
        j["anti_swears"] = anti_swears;

        return j;
    }


    static Guild from_json(nlohmann::json& j) {
        Guild g;

        g.guild_id = j.value("guild_id", 0ULL);

        // users
        for (const auto& u : j["users"]) {
            g.add_user(User::from_json(u));
        }

        // banned_ids
        for (const auto& id : j["banned_ids"])
            g.add_banned_id(id.get<uint64_t>());

        // admin_ids
        for (const auto& id : j["admin_ids"])
            g.add_admins_id(id.get<uint64_t>());

        // banned_channels
        for (const auto& id : j["banned_channels"])
            g.add_banned_channel(id.get<uint64_t>());

        // tts_channels
        for (const auto& id : j["tts_channels"])
            g.add_tts_channels(id.get<uint64_t>());

        // lvl_roles
        if (j.contains("lvl_roles") && j["lvl_roles"].is_array()) {
            for (const auto& item : j["lvl_roles"]) {
                lvl_role r;

                r.id_role = item.value("id_role", 0ULL);
                r.xp_role = item.value("xp_role", 0);
                r.type = item.value("type", "");

                g.lvl_roles.push_back(r);
            }
        }
        // auto_reply
        if (j.contains("auto_reply") && j["auto_reply"].is_object())
        {
            for (auto it = j["auto_reply"].begin(); it != j["auto_reply"].end(); ++it)
            {
                AutoReplyData data;

                data.message = it.value().value("message", "");
                data.channel_id = it.value().value("channel_id", 0ULL);

                g.auto_reply[it.key()] = data;
            }
        }

        // banned_words
        if (j.contains("banned_words") && j["banned_words"].is_array()) {
            for (const auto& word : j["banned_words"])
                g.add_banned_word(word);
        }
        g.anti_swears = j.value("anti_swears", false);

        return g;
    }

	bool has_user(dpp::snowflake user_id);
	void add_user(const User& u);
};