#pragma once
#include "User.h"
#include <iostream>
#include <dpp/dpp.h>
#include <vector>
#include <string>

class Guild {
private:
	dpp::snowflake guild_id;
	std::vector<dpp::snowflake> banned_ids;
	std::vector<dpp::snowflake> admin_ids;
	std::vector<dpp::snowflake> banned_channels;
	std::vector<dpp::snowflake> tss_channels;
	std::vector<User> users;
public:

	void add_banned_id(dpp::snowflake banned_id);
	void remove_banned_id(dpp::snowflake banned_id);
	void add_admins_id(dpp::snowflake admin_id);
	void remove_admins_id(dpp::snowflake admin_id);
	void add_banned_channel(dpp::snowflake banned_channel);
	void remove_banned_channel(dpp::snowflake banned_channel);
	void add_tts_channels(dpp::snowflake tts_chanel);
	void remove_tts_channels(dpp::snowflake tts_chanel);
};