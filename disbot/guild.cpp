#include "guild.h"
#include "User.h"
#include <nlohmann/json.hpp>

void Guild::create_guild(dpp::snowflake guild_id, std::unordered_set<dpp::snowflake> banned_ids, std::unordered_set<dpp::snowflake> admin_ids, std::unordered_set<dpp::snowflake> banned_channels, std::unordered_set<dpp::snowflake> tts_channels, std::unordered_map<dpp::snowflake, User> users, std::unordered_set<std::string> banned_words, std::unordered_map<std::string, AutoReplyData> auto_reply, std::vector<lvl_role> lvl_roles){
	this->guild_id = guild_id;
	this->banned_ids = banned_ids;
	this->admin_ids = admin_ids;
	this->banned_channels = banned_channels;
	this->tts_channels = tts_channels;
	this->users = users;
	this->banned_channels = banned_channels;
	this->auto_reply = auto_reply;
	this->lvl_roles = lvl_roles;
}

void Guild::add_message_history(const std::string& str, dpp::snowflake channel_id)
{
    auto& dq = chat_history[channel_id];

    dq.push_front(str);

    if (dq.size() > 20) {
        dq.pop_back();
    }
}
std::deque<std::string> Guild::get_all_channel_history(dpp::snowflake channel_id)
{
	auto it = chat_history.find(channel_id);

	if (it != chat_history.end())
		return it->second;

	return {};
}
std::unordered_map<dpp::snowflake, std::deque<std::string>> Guild::get_all_chat_history()
{
	return std::unordered_map<dpp::snowflake, std::deque<std::string>>(chat_history);
}

void Guild::add_auto_reply(std::string key_word, std::string message, dpp::snowflake channel)
{
	if (!auto_reply.contains(key_word)) {
		AutoReplyData data;
		data.channel_id = channel;
		data.message = message;
		auto_reply[key_word] = data;
	}
}

bool Guild::remove_auto_reply(std::string key_word)
{
	if (auto_reply.contains(key_word)) {
		auto_reply.erase(key_word);
		return true;
	}
	else return false;
}

void Guild::add_banned_id(dpp::snowflake banned_id){
	if (!banned_ids.contains(banned_id)) {
		banned_ids.insert(banned_id);
	}
}

bool Guild::remove_banned_id(dpp::snowflake banned_id)
{
	if (banned_ids.contains(banned_id)) {
		banned_ids.erase(banned_id);
		return true;
	}
	else return false;
}

void Guild::add_admins_id(dpp::snowflake admin_id) {
	if (!admin_ids.contains(admin_id)) {
		admin_ids.insert(admin_id);
	}
}

bool Guild::remove_admins_id(dpp::snowflake admin_id)
{
	if (admin_ids.contains(admin_id)) {
		admin_ids.erase(admin_id);
		return true;
	}
	return false;
}

void Guild::add_banned_channel(dpp::snowflake banned_channel) {
	if (!banned_channels.contains(banned_channel)) {
		banned_channels.insert(banned_channel);
	}	
}

bool Guild::remove_banned_channel(dpp::snowflake banned_channel)
{
	if (banned_channels.contains(banned_channel)) {
		banned_channels.erase(banned_channel);
		return true;
	}
	return false;
}

bool Guild::is_banned_channel(dpp::snowflake banned_channel)
{
	if (banned_channels.contains(banned_channel)) {
		return true;
	}
	return false;
}

void Guild::add_tts_channels(dpp::snowflake tts_channel){
	if (!tts_channels.contains(tts_channel)) {
		tts_channels.insert(tts_channel);
	}
}

bool Guild::remove_tts_channels(dpp::snowflake tts_channel)
{
	if (tts_channels.contains(tts_channel)) {
		tts_channels.erase(tts_channel);
		return true;
	}
	return false;
}

void Guild::add_lvl_role(dpp::snowflake id_role, int exp_role, std::string type)
{
	lvl_roles.push_back({ id_role, exp_role, type });
}

void Guild::remove_lvl_role(dpp::snowflake id_role) {
	auto it = std::find_if(lvl_roles.begin(), lvl_roles.end(),
		[&](const lvl_role& r) {
			return r.id_role == id_role;
		}
	);

	if (it != lvl_roles.end()) {
		lvl_roles.erase(it);
	}
}

bool Guild::add_banned_word(std::string ban_word)
{
	if (!banned_words.contains(ban_word)) {
		banned_words.insert(ban_word);
		return true;
	}
	return false;
}

bool Guild::remove_banned_word(std::string ban_word)
{
	if (banned_words.contains(ban_word)) {
		banned_words.erase(ban_word);
		return true;
	}
	return false;
}

bool Guild::has_banned_word(std::string ban_word)
{
	for (auto& word : banned_words) {
		if (ban_word.find(word) != std::string::npos) {
			return true;
		}
	}
	return false;
}


void Guild::anti_swear(bool bul)
{
	anti_swears = bul;
}

bool Guild::is_auto_reply_word(std::string word, dpp::snowflake channel)
{
	/*for (auto& [key_word, message] : auto_reply) {
		if (word.find(key_word) != std::string::npos) {
			return true;
		}
	}
	*/
	if (auto_reply.contains(word))
		if (auto_reply[word].channel_id == channel)
		return true;
	return false;
}

bool Guild::is_anti_swear()
{
	return anti_swears;
}

std::string Guild::get_auto_reply_message(std::string key_word)
{
	if (auto_reply.contains(key_word)) {
		return auto_reply[key_word].message;
	}
	return std::string("");
}


User* Guild::get_user(dpp::snowflake user_id)
{
	if (users.contains(user_id)) {
		auto it = users.find(user_id);
		if (it != users.end())
			return &it->second;
		return nullptr;
	}
	else {
		User u;
		
		u.Create_user(user_id);
		users[u.get_user_id()] = u;
		std::cout << "created user " << u.get_user_id() << "\n";
		auto it = users.find(user_id);
		if (it != users.end())
			return &it->second;
		return nullptr;
	}
}



bool Guild::remove_user(dpp::snowflake user_id)
{
	if (users.contains(user_id)) {
		users.erase(user_id);
		return true;
	}
	return false;
}




bool Guild::has_user(dpp::snowflake user_id)
{
	if (users.contains(user_id)) {
		return true;
	}else return false;
}

void Guild::add_user(const User& u) {
	users[u.get_user_id()] = u;
}

