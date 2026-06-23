#pragma once
#include <dpp/dpp.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <set>
#include <unordered_set>

class User {

private:
	dpp::snowflake user_id;
	int exp_text;
	int exp_voice;
	int time_muted;
	int exp_swears;
	std::unordered_map<int, std::string> warns;
	std::unordered_set<dpp::snowflake> roles_id;
	bool banned;
	bool admin;
	std::string tts_voice;
	bool tts_enable;
	bool moderate_text;
public:
	void Create_user(dpp::snowflake user_id = 0, int exp_text = 0, int exp_voice = 0, int time_muted = 0, std::unordered_map<int, std::string> warns = {}, bool banned = false, bool admin = false, std::string tts_voice = "&hl=ru-ru&v=Peter", bool tts_enable = false, bool moderate_text = true, std::unordered_set<dpp::snowflake> roles_id = {}, int exp_swears = 0);
	void Add_exp_text(int exp);
	void Remove_exp_text(int exp);
	void Add_exp_voice(int exp);
	void Remove_exp_voice(int exp);
	void Add_time_muted(int exp);
	void Remove_time_muted(int exp);
	void Add_admin();
	void Remove_admin();
	void Add_ban();
	void Remove_ban();
	void Add_exp_swears(int exp);
	void Remove_exp_swears(int exp);
	bool add_warn(std::string reason = "warned");
	bool remove_warn(int warn = 1, std::string reason = "warned");
	void tts_enable_change(bool booled) { tts_enable = booled; }
	void moderate_text_change(bool booled) { moderate_text = booled; }
	void tts_voice_change(std::string voice);
	dpp::snowflake get_user_id() const;
	int get_user_exp_text() const;
	int get_user_exp_voice() const;
	int get_time_muted() const;
	std::string get_user_tts_voice() const;
	int get_user_exp_swears() const;
	std::unordered_map<int, std::string> get_warns();
	bool is_tts_enable() const { return tts_enable; }
	bool is_moderate_text_enable() const { return moderate_text; }
	bool is_banned();
	bool is_admin();
	void add_role(dpp::snowflake id);
	void remove_role(dpp::snowflake id);
	std::unordered_set<dpp::snowflake> get_roles() const { return roles_id; }
	bool has_role(dpp::snowflake id) const { return roles_id.contains(id); }

	nlohmann::json to_json() const {
		nlohmann::json warns_json = nlohmann::json::object();

		for (const auto& [id, text] : warns) {
			warns_json[std::to_string(id)] = text;
		}

		nlohmann::json j;
		j["user_id"] = static_cast<uint64_t>(user_id);
		j["exp_text"] = exp_text;
		j["exp_voice"] = exp_voice;
		j["time_muted"] = time_muted;
		j["warns"] = warns_json;
		j["banned"] = banned;
		j["admin"] = admin;
		j["tts_voice"] = tts_voice;
		j["tts_enable"] = tts_enable;
		j["moderate_text"] = moderate_text;
		j["exp_swears"] = exp_swears;
		for (const auto& id : roles_id)
			j["roles_id"].push_back(static_cast<uint64_t>(id));

		return j;
	}


	static User from_json(const nlohmann::json& j) {
		User u;

		u.user_id = j.value("user_id", 0ULL);
		u.exp_text = j.value("exp_text", 0);
		u.exp_voice = j.value("exp_voice", 0);
		u.time_muted = j.value("time_muted", 0);
		u.exp_swears = j.value("exp_swears", 0);
		u.banned = j.value("banned", false);
		u.admin = j.value("admin", false);
		u.tts_voice = j.value("tts_voice", "&hl=ru-ru&v=Peter");
		u.tts_enable = j.value("tts_enable", false);
		u.moderate_text = j.value("moderate_text", true);

		if (j.contains("warns") && j["warns"].is_object()) {
			for (auto it = j["warns"].begin(); it != j["warns"].end(); ++it) {
				int warn_id = std::stoi(it.key());
				std::string text = it.value().get<std::string>();

				u.warns[warn_id] = text;
			}
		}
		if (j.contains("roles_id") && j["roles_id"].is_array()) {
			for (const auto& id : j["roles_id"])
				u.add_role(id.get<uint64_t>());
		}
		return u;
	}

};
