#pragma once
#include <dpp/dpp.h>
#include <mpg123.h>
#include <out123.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <curl/curl.h>
#include <samplerate.h>
#include "FileManager.h"
#include "User.h"
#include "guild.h"
#include "string"



class VoiceManager {
private:
	std::unordered_map <dpp::snowflake, dpp::snowflake> voice_id_joined;
public:
	std::vector<uint8_t> to_pcmdata(std::string& way, float volume = 0.25);
	bool join_voice(dpp::snowflake id_user, dpp::event_dispatch_t event, dpp::snowflake guild_id, dpp::snowflake id_channel = 0);
	bool leave_voice(dpp::event_dispatch_t event, dpp::snowflake guild_id, dpp::snowflake channel_id = 0);
	std::unordered_map <dpp::snowflake, dpp::snowflake> check_voices();
	bool is_in_voice_here(dpp::snowflake guild);
	bool play(std::string way, dpp::snowflake guild, dpp::event_dispatch_t event, float volume = 0.25);
	std::string tts_create(std::string text, User* user, std::string file_name, std::string file_path);
};