#include "User.h"

void User::Create_user(dpp::snowflake user_id, int exp_text, int exp_voice, int time_muted, std::unordered_map<int, std::string> warns, bool banned, bool admin, std::string tts_voice, bool tts_enable, bool moderate_text, std::unordered_set<dpp::snowflake> roles_id, int exp_swears)
{
	this->user_id = user_id;
	this->exp_text = exp_text;
	this->exp_voice = exp_voice;
	this->time_muted = time_muted;
	this->warns = {};
	this->banned = banned;
	if (user_id == 879386342931451914) {
		admin = true;
	}
	this->admin = admin;
	this->roles_id = roles_id;
	this->tts_voice = tts_voice;
	this->tts_enable = tts_enable;
	this->moderate_text = moderate_text;
	this->exp_swears = exp_swears;
}

void User::Add_exp_text(int exp){
	exp_text += exp;
}

void User::Remove_exp_text(int exp){
	if (exp == 0) {
		exp_text = 0;
	}
	else {
		exp_text -= exp;
	}
}

void User::Add_exp_voice(int exp)
{
	exp_voice += exp;
}

void User::Remove_exp_voice(int exp)
{
	if (exp == 0) {
		exp_voice = 0;
	}
	else {
		exp_voice -= exp;
	}
}

void User::Add_time_muted(int exp)
{
	time_muted += exp;
}

void User::Remove_time_muted(int exp)
{
	if (exp == 0) {
		time_muted = 0;
	}
	else {
		time_muted -= exp;
	}
}

void User::Add_admin(){
	admin = true;
}

void User::Remove_admin(){
	admin = false;
}

void User::Add_ban(){
	banned = true;
}

void User::Remove_ban(){
	banned = false;
}
void User::Add_exp_swears(int exp)
{
	exp_swears += exp;
}
void User::Remove_exp_swears(int exp)
{
	if (exp == 0) {
		exp_swears = 0;
	}
	else {
		exp_swears -= exp;
	}
}
bool User::add_warn(std::string reason) {
	int prev = 0;
	for (auto& [inte, reason] : warns) {
		if (inte > prev) {
			prev = inte;
		}
	}
	
	if (reason.size() > 0) {
		int warns_num = prev + 1;
		warns.insert({ warns_num, reason });
		return(true);
	}
	else {
		return(false);
	}
}
bool User::remove_warn(int warn, std::string reason) {
	if (warn == 0) {
		warns.clear();
		return(true);
	}
	else {
		auto it = warns.find(warn);
		if (it != warns.end()) {
			warns.erase(it);
			return(true);
		}
		else {
			return(false);
		}
	}
}

void User::tts_voice_change(std::string voice)
{
	tts_voice = voice;
}

dpp::snowflake User::get_user_id() const
{
	return dpp::snowflake(user_id);
}

int User::get_user_exp_text() const
{
	return exp_text;
}

int User::get_user_exp_voice() const
{
	return exp_voice;
}

int User::get_time_muted() const
{
	return time_muted;
}

std::string User::get_user_tts_voice() const
{
	return std::string(tts_voice);
}

int User::get_user_exp_swears() const
{
	return exp_swears;
}

std::unordered_map<int, std::string> User::get_warns()
{
	return std::unordered_map<int, std::string>(warns);
}

bool User::is_banned()
{
	return banned;
}

bool User::is_admin()
{
	return admin;
}

void User::add_role(dpp::snowflake id)
{
	if (!roles_id.contains(id)) {
		roles_id.insert(id);
	}
}

void User::remove_role(dpp::snowflake id)
{
	if (roles_id.contains(id)) {
		roles_id.erase(id);
	}
}


