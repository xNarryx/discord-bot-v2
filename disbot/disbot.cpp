#include <iostream>
#include <dpp/dpp.h>
#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <shared_mutex>
#include <sstream>
#include <iomanip>
#include <ctime>
#include "FileManager.h"
#include "guild.h"
#include "User.h"
#include "VoiceManager.h"

file_manager fm;
VoiceManager v;
std::unordered_map<std::string, std::function<void(const dpp::slashcommand_t&)>> handlers_cmd;
std::unordered_set<std::string> swears;

std::string to_utf8(const std::wstring& wstr) {
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string str_to(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str_to[0], size_needed, NULL, NULL);
	return str_to;
}
std::wstring string_to_wstring(const std::string& str) {
	int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	if (len == 0) return L"";
	std::wstring wstr(len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], len);
	wstr.pop_back();
	return wstr;
}
std::string wstring_to_string(const std::wstring& wstr) {
	int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (len == 0) return "";
	std::string str(len, '\0');
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], len, nullptr, nullptr);
	str.pop_back();
	return str;
}
std::string to_lower_utf8(const std::string& input) {
	std::wstring wstr = string_to_wstring(input);
	std::locale loc("ru_RU.UTF-8");
	for (auto& ch : wstr) {
		ch = std::tolower(ch, loc);
	}
	return wstring_to_string(wstr);
}
std::string getCurrentDateTime() {
	auto now = std::chrono::system_clock::now();
	std::time_t now_time = std::chrono::system_clock::to_time_t(now);

	std::tm timeinfo;
	localtime_s(&timeinfo, &now_time);

	char buffer[20];
	std::strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M", &timeinfo);

	return std::string(buffer);
}
void SetColor(int color = 7) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}
std::vector<std::string> split(std::string str) {
	std::string word;
	std::vector<std::string> splited;
	for (const char ch : str) {
		if (ch == ' ') {
			splited.push_back(word);
			word = "";
		}
		else {
			word += ch;
		}
	}
	if (!word.empty()) {
		splited.push_back(word);
	}
	return splited;
}
std::string normalize(const std::string str) {
	std::unordered_set<char> chars = { '.', ',', ';', '\'', '[',']', '-', '=','_','+', '!', '@', '#' , '$', '%', '^', '&', '?', '*', '(', ')', '~', '`', ';', ':', '<', '>', '\\', '|' , '/', '{', '}' };
	/*std::unordered_map<char, char> letters = {
		{'0', 'о'}, 
		{'4', 'е'},
		{'e', 'е'}, 
		{'l', 'л'}, 
		{'a', 'а'}, 
		{'o', 'о'}, 
		{'y', 'у'}, 
		{'і', 'ы'}, 
		{'7', 'ч'}
	};*/
	std::string string, strings;
	for (auto& ch : str) {
		if (!chars.contains(ch)) {
			string += ch;
		}
	}
	/*for (auto& ch : to_utf8(string_to_wstring(string))) {
		std::cout << "Гандон буква: " << ch << "\n";
		if (letters.contains(ch)) {
			strings += letters[ch];
		}
		else {
			strings += ch;
		}
	} */
	return string;
}
void load_swears(const std::string& filename) {
	std::ifstream file(filename);

	if (!file.is_open()) {
		return;
	}

	std::string line;
	while (std::getline(file, line)) {
		if (!line.empty()) {
			swears.insert(line);
		}
	}
}

void bot_thread(dpp::cluster& bot){
	std::thread([&bot]() {
		while (true) {
			std::this_thread::sleep_for(std::chrono::seconds(15));
			for (auto& [it, g] : fm.get_guilds()) {
				if (g.get_id() != 0) {
					dpp::guild* g_l = dpp::find_guild(g.get_id());
					for (auto& [user_id, state] : g_l->voice_members) {
						if (!state.is_self_mute()) {
							auto* u = g.get_user(user_id);
							u->Add_exp_voice(1);
						}
						else {
							auto* u = g.get_user(user_id);
							u->Add_time_muted(1);
						}
					}
				}
			}

		}
		}).detach();

}
void bot_thread_saver(dpp::cluster& bot) {
	std::thread([&bot]() {
		while (true) {
			std::this_thread::sleep_for(std::chrono::seconds(300));
			if (fm.save_guilds("D:\\DEV\\Disbot\\tests\\")) {
				SetColor(2);
				std::cout << getCurrentDateTime() << " data saved.\n";
				SetColor();
			}
			else {
				SetColor(4);
				std::cout << getCurrentDateTime() << " data was not saved.\n";
				SetColor();
			}
		}
		}).detach();
}
void bot_thread_exp_lvls(dpp::cluster& bot) {
	std::thread([&bot]() {
		while (true) {
			std::this_thread::sleep_for(std::chrono::minutes(5));
			Guild g;
			User u;
			for (auto& [guild_id, g] : fm.get_guilds_r()) {
				for (auto& [user_id , u] : g.get_users()) {
					for (auto& role : g.get_lvl_roles()) {
						if (!u.has_role(role.id_role)) {
							if (role.type == "exp_text") {
								if (u.get_user_exp_text() >= role.xp_role) {
									bot.guild_member_add_role(g.get_id(), user_id, role.id_role);
									Guild& gl = fm.get_guild(guild_id);
									User* us = gl.get_user(user_id);
									us->add_role(role.id_role);
									SetColor(6);
									std::cout << "Gived role " << g.get_id() << " " << user_id << " " << role.id_role << "\n";
									SetColor();
								}
							}
							else if (role.type == "exp_voice") {
								if (u.get_user_exp_voice() >= role.xp_role) {
									bot.guild_member_add_role(g.get_id(), user_id, role.id_role);
									Guild& gl = fm.get_guild(guild_id);
									User* us = gl.get_user(user_id);
									us->add_role(role.id_role);
									SetColor(6);
									std::cout << "Gived role " << g.get_id() << " " << user_id << " " << role.id_role << "\n";
									SetColor();
								}
							}
							else if (role.type == "exp_muted") {
								if (u.get_time_muted() >= role.xp_role) {
									bot.guild_member_add_role(g.get_id(), user_id, role.id_role);
									Guild& gl = fm.get_guild(guild_id);
									User* us = gl.get_user(user_id);
									us->add_role(role.id_role);
									SetColor(6);
									std::cout << "Gived role " << g.get_id() << " " << user_id << " " << role.id_role << "\n";
									SetColor();
								}
							}
							else if (role.type == "exp_all") {
								int all_exp = u.get_time_muted() + u.get_user_exp_text() + u.get_user_exp_voice();
								if (all_exp >= role.xp_role) {
									bot.guild_member_add_role(g.get_id(), user_id, role.id_role);
									Guild& gl = fm.get_guild(guild_id);
									User* us = gl.get_user(user_id);
									us->add_role(role.id_role);
									SetColor(6);
									std::cout << "Gived role " << g.get_id() << " " << user_id << " " << role.id_role << "\n";
									SetColor();
								}
							}
						}
					}
				}
			}
		}
		}).detach();
}

std::vector<dpp::slashcommand> build_commands(dpp::snowflake bot_id) {
	std::vector<dpp::slashcommand> cmds;

#pragma region Say
	dpp::slashcommand say(
		"say",
		to_utf8(L"произнести в чате"),
		bot_id
	);
	say.add_option(
		dpp::command_option(
			dpp::co_string,
			"text",
			to_utf8(L"Введите сообщение"),
			false
		)
	);
	say.add_option(
		dpp::command_option(
			dpp::co_attachment,
			"attachment",
			to_utf8(L"Дополнительный файл"),
			false
		)
	);
	cmds.push_back(say);
#pragma endregion
#pragma region Leaders
	dpp::slashcommand leaders(
		"leaderscore",
		to_utf8(L"Список лидеров"),
		bot_id
	);

	
	leaders.add_option(
		dpp::command_option(
			dpp::co_string,    
			"type",                  
			to_utf8(L"Выберите критерий лидерборда"),
			true                     
		)
		.add_choice(dpp::command_option_choice(to_utf8(L"Опыт в чате"), "xp_text"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Опыт в войсе"), "xp_voice"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Время мута"), "mute_time"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Количесто матершиницы"), "xp_swear"))

	);
	leaders.add_option(
		dpp::command_option(
			dpp::co_string,
			"large",
			to_utf8(L"Выберите длину списка"),
			true
		)
		.add_choice(dpp::command_option_choice(to_utf8(L"10 пользователей"), "10"))
		.add_choice(dpp::command_option_choice(to_utf8(L"20 пользователей"), "20"))
		.add_choice(dpp::command_option_choice(to_utf8(L"30 пользователей"), "30"))

	);
	cmds.push_back(leaders);
#pragma endregion
#pragma region Add_role_exp
	dpp::slashcommand add_role_exp(
		"add_role_exp",
		to_utf8(L"Добавить роль которую можна получить за опыт"),
		bot_id
	);

	add_role_exp.add_option(
		dpp::command_option(
			dpp::co_role,
			"role",
			to_utf8(L"Введите роль для добавления"),
			true
		)
	);
	add_role_exp.add_option(
		dpp::command_option(
			dpp::co_integer,
			"exp",
			to_utf8(L"Введите количество опыта для получения"),
			true
		)
	);
	add_role_exp.add_option(
		dpp::command_option(
			dpp::co_string,
			"type",
			to_utf8(L"Выберите длину списка"),
			true
		)
		.add_choice(dpp::command_option_choice(to_utf8(L"За опыт чатов"), "exp_text"))
		.add_choice(dpp::command_option_choice(to_utf8(L"За опыт голосовых чатов"), "exp_voice"))
		.add_choice(dpp::command_option_choice(to_utf8(L"За замьюченый опыт"), "exp_muted"))
		.add_choice(dpp::command_option_choice(to_utf8(L"За весь опыт"), "exp_all"))

	);
	cmds.push_back(add_role_exp);
#pragma endregion
#pragma region role_list
	// role_list
	dpp::slashcommand role_list(
		"role_list",
		to_utf8(L"Выводит список ролей"),
		bot_id
	);
	cmds.push_back(role_list);

	dpp::slashcommand remove_role(
		"remove_role",
		to_utf8(L"Убрать роль"),
		bot_id
	);
	remove_role.add_option(
		dpp::command_option(
			dpp::co_role,
			"role",
			to_utf8(L"Введите роль для удаления"),
			true
		)
	);
	cmds.push_back(remove_role);
#pragma endregion
#pragma region Add_admin
	dpp::slashcommand add_admin(
		"add_admin",
		to_utf8(L"Добавить администратора"),
		bot_id
	);
	add_admin.add_option(
		dpp::command_option(
			dpp::co_user,
			"user",
			to_utf8(L"Введите пользователя"),
			true
		)
	);
	cmds.push_back(add_admin);
#pragma endregion
#pragma region Remove_admin
	dpp::slashcommand remove_admin(
		"remove_admin",
		to_utf8(L"Удалить администратора"),
		bot_id
	);
	remove_admin.add_option(
		dpp::command_option(
			dpp::co_user,
			"user",
			to_utf8(L"Введите пользователя"),
			true
		)
	);
	cmds.push_back(remove_admin);
#pragma endregion
#pragma region Admin_list
	dpp::slashcommand admin_list(
		"admin_list",
		to_utf8(L"Список администраторов"),
		bot_id
	);
	cmds.push_back(admin_list);
#pragma endregion
#pragma region Warn
	dpp::slashcommand warn(
		"warn",
		to_utf8(L"Предупредить пользователя"),
		bot_id
	);
	warn.add_option(
		dpp::command_option(
			dpp::co_user,
			"user",
			to_utf8(L"Введите пользователя"),
			true
		)

	);
	warn.add_option(
		dpp::command_option(
			dpp::co_string,
			"reason",
			to_utf8(L"Введите причину предупреждения"),
			true
		)

	);
	cmds.push_back(warn);
#pragma endregion
#pragma region Warn_check
	dpp::slashcommand warn_check(
		"warn_check",
		to_utf8(L"Посмотреть предупреждения пользователя"),
		bot_id
	);
	warn_check.add_option(
		dpp::command_option(
			dpp::co_user,
			"user",
			to_utf8(L"Введите пользователя"),
			true
		)

	);
	cmds.push_back(warn_check);
#pragma endregion
#pragma region Warn_Remove
	dpp::slashcommand warn_remove(
		"warn_remove",
		to_utf8(L"Убрать предупреждения пользователя"),
		bot_id
	);
	warn_remove.add_option(
		dpp::command_option(
			dpp::co_user,
			"user",
			to_utf8(L"Введите пользователя"),
			true
		)
	);
	warn_remove.add_option(
		dpp::command_option(dpp::co_string, "warn_id", "Warn number", true)
		.set_auto_complete(true)
	);
	cmds.push_back(warn_remove);
#pragma endregion
#pragma region ban_word
	dpp::slashcommand ban_word(
		"ban_word",
		to_utf8(L"Бан-ворды"),
		bot_id
	);
	ban_word.add_option(
		dpp::command_option(
			dpp::co_string,
			"type",
			to_utf8(L"Выберите критерий Бан-ворда"),
			true
		)
		.add_choice(dpp::command_option_choice(to_utf8(L"Добавить бан-ворд"), "add_ban"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Убрать бан-ворд"), "remove_ban"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Добавить не модериуемый канал"), "add_ban_channel"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Убрать не модериуемый канал"), "remove_ban_channel"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Добавить не модерируемого пользователя"), "add_nomod_user"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Убрать не модерируемого пользователя"), "remove_nomod_user"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Список бан-вордов"), "list"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Список не модерируемых каналов"), "list_channel"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Проверить, модерируется ли пользователь"), "user"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Переключить удаление матов на сервере"), "swears"))

	);
	ban_word.add_option(
		dpp::command_option(
			dpp::co_string,
			"word",
			to_utf8(L"Обязательно при удалении, или добавлении бан-ворда"),
			false
		)
	);
	ban_word.add_option(
		dpp::command_option(
			dpp::co_channel,
			"channel",
			to_utf8(L"Обязательно при удалении, или добавлении не модерируемого канала"),
			false
		)
	);
	ban_word.add_option(
		dpp::command_option(
			dpp::co_user,
			"user",
			to_utf8(L"Обязательно при добавлении не модерируемого пользователя"),
			false
		)
	);
		
	cmds.push_back(ban_word);
#pragma endregion
#pragma region TTS
	dpp::slashcommand tts(
		"tts",
		to_utf8(L"TTS управление озвучкой"),
		bot_id
	);

	tts.add_option(
		dpp::command_option(
			dpp::co_string,
			"action",
			to_utf8(L"Выберите действие"),
			true
		)
		.add_choice(dpp::command_option_choice(to_utf8(L"Включить TTS"), "enable"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Выключить TTS"), "disable"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Сменить голос"), "voice"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Озвучить текст"), "say"))
	);

	tts.add_option(
		dpp::command_option(
			dpp::co_string,
			"voice_change",
			to_utf8(L"Выберите голос для озвучки"),
			false
		)
		.add_choice(dpp::command_option_choice(to_utf8(L"Женский голос 1 (тестовый)"), "&hl=ru-ru&v=Olga"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Женсикй голос 2"), "&hl=ru-ru&v=Marina"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Мужской голос 1"), "&hl=ru-ru&v=Peter"))
	);

	tts.add_option(
		dpp::command_option(
			dpp::co_string,
			"text",
			to_utf8(L"Текст для озвучки"),
			false
		)
	);

	cmds.push_back(tts);
#pragma endregion
	return cmds;
}
void load_commads(dpp::cluster& bot) {
	handlers_cmd["say"] = [&](const dpp::slashcommand_t& event) {
		std::string text = " ";
		Guild gl;
		gl = fm.get_guild_r(event.command.guild_id);
		User* uu = gl.get_user(event.command.usr.id);
		if (uu->is_admin()) {
			auto param = event.get_parameter("text");
			if (param.index() != 0) {
				std::string text = std::get<std::string>(event.get_parameter("text"));
			}
				dpp::command_interaction cmd_data = std::get<dpp::command_interaction>(event.command.data);

				param = event.get_parameter("attachment");
				if (param.index() != 0) {
					dpp::snowflake file_id = std::get<dpp::snowflake>(event.get_parameter("attachment"));
					auto iter = event.command.resolved.attachments.find(file_id);
					if (iter != event.command.resolved.attachments.end()) {
						const dpp::attachment& att = iter->second;
						bot.message_create(dpp::message(event.command.channel_id, text + " " + att.url));
						event.reply(
							dpp::message(to_utf8(L"Отправила")).set_flags(dpp::m_ephemeral)
						);
					}
				}
				else {
					event.reply(
						dpp::message(to_utf8(L"Отправила")).set_flags(dpp::m_ephemeral)
					);
					bot.message_create(dpp::message(event.command.channel_id, text));
				}
			
		}
		else {
			event.reply(
				dpp::message(to_utf8(L"У вас нет прав на это действие")).set_flags(dpp::m_ephemeral)
			);
		}
		};

	handlers_cmd["tts"] = [&](const dpp::slashcommand_t& event) {
		dpp::snowflake guild_id = event.command.guild_id;
		dpp::snowflake user_id = event.command.usr.id;

		std::string action = std::get<std::string>(event.get_parameter("action")); 

		if (action == "enable") {
			Guild& g = fm.get_guild(event.command.guild_id);
			User* u = g.get_user(user_id);
			u->tts_enable_change(true);
			event.reply(dpp::message(to_utf8(L"Включила озвучку ваших сообщений.")).set_flags(dpp::m_ephemeral));
		}
		else if (action == "disable") {
			Guild& g = fm.get_guild(event.command.guild_id);
			User* u = g.get_user(user_id);
			u->tts_enable_change(false);
			event.reply(dpp::message(to_utf8(L"Выключила озвучку ваших сообщений.")).set_flags(dpp::m_ephemeral));
		}
		else if (action == "voice") {
			std::string voice_change = std::get<std::string>(event.get_parameter("voice_change"));
			Guild& g = fm.get_guild(event.command.guild_id);
			User* u = g.get_user(user_id);
			u->tts_voice_change(voice_change);
			event.reply(dpp::message(to_utf8(L"Сменила ваш голос.")).set_flags(dpp::m_ephemeral));
		}
		else if (action == "say") {
			std::string text = std::get<std::string>(event.get_parameter("text"));
			Guild& guild = fm.get_guild_r(guild_id);
			if (v.is_in_voice_here(guild_id)) {
				v.play(v.tts_create(text, guild.get_user(event.command.usr.id), std::to_string(guild_id), "D:\\DEV\\Disbot\\tts\\"), guild_id, event);
				event.reply(dpp::message(to_utf8(L"Озвучила ваш текст ") + text).set_flags(dpp::m_ephemeral));
			}
			else {
				event.reply(dpp::message(to_utf8(L"Я не в голосовом канале.")).set_flags(dpp::m_ephemeral));
			}
		}
		else {
			event.reply(to_utf8(L"Неизвестное действие"));
		}
		};

	handlers_cmd["ban_word"] = [&](const dpp::slashcommand_t& event) {
		Guild gl;
		gl = fm.get_guild_r(event.command.guild_id);
		User* uu = gl.get_user(event.command.usr.id);
		if (uu->is_admin()) {
			std::string type = std::get<std::string>(event.get_parameter("type"));
				if (type == "add_ban") {
					std::string word = std::get<std::string>(event.get_parameter("word"));
					Guild& g = fm.get_guild(event.command.guild_id);
					g.add_banned_word(word);
					event.reply(to_utf8(L"Добавила бан-ворд!"));
				}
				else if (type == "remove_ban") {
					std::string word = std::get<std::string>(event.get_parameter("word"));
					Guild& g = fm.get_guild(event.command.guild_id);
					if (g.has_banned_word(word)) {
						if (g.remove_banned_word(word)) {
							event.reply(to_utf8(L"Удалила бан-ворд!"));
						}
						else {
							event.reply(to_utf8(L"Что-то пошло не так!"));
						}
					}
					else {
						event.reply(to_utf8(L"Не нашла этот бан-ворд!"));
					}
				}
				else if (type == "list") {
					std::vector<std::string> ban_word_list;
					std::string reply = to_utf8(L"Список бан-вордов:\n```");
					for (const auto& word : gl.get_banned_words()) {
						reply += word + "\n";
						if (reply.size() > 500) {
							reply += "```";
							ban_word_list.push_back(reply);
							reply = "```";
						}
					}
					reply += "```";
					ban_word_list.push_back(reply);
					for (const auto& words : ban_word_list) {
						bot.message_create(dpp::message(event.command.channel_id, words));
					}
					
				}
				else if (type == "swears") {
					Guild& g = fm.get_guild(event.command.guild_id);
					if (g.is_anti_swear()) {
						g.anti_swear(false);
						event.reply(dpp::message(to_utf8(L"Выключила удаление матов!")).set_flags(dpp::m_ephemeral));

					}
					else {
						g.anti_swear(true);
						event.reply(dpp::message(to_utf8(L"Включила удаление матов!")).set_flags(dpp::m_ephemeral));

					}
				}
				else if (type == "add_ban_channel") {
					dpp::snowflake channel = std::get<dpp::snowflake>(event.get_parameter("channel"));
					Guild& g = fm.get_guild(event.command.guild_id);

					g.add_banned_channel(channel);
					event.reply(to_utf8(L"Добавила не модерируемый канал!"));
						
				
				}
				else if (type == "remove_ban_channel") {
					dpp::snowflake channel = std::get<dpp::snowflake>(event.get_parameter("channel"));
					Guild& g = fm.get_guild(event.command.guild_id);

					if (g.remove_banned_channel(channel)) {
						event.reply(to_utf8(L"Убрала не модерируемый канал!"));
					}
					else {
						event.reply(to_utf8(L"Что-то пошло не так!"));
					}

				}
				else if (type == "list_channel") {
					Guild& g = fm.get_guild_r(event.command.guild_id);
					std::string reply = "**Не модерируемые каналы на текущий момент:**\n";
					for (auto channel : g.get_banned_channel_list()) {
						reply = reply + "<#" + std::to_string(channel) + ">\n";
					}
					event.reply(reply);

				}
				else if (type == "user") {
					Guild& g = fm.get_guild_r(event.command.guild_id);
					dpp::snowflake user = std::get<dpp::snowflake>(event.get_parameter("user"));
					User* u = g.get_user(user);
					if (u->is_moderate_text_enable()) {
						event.reply("Пользователь модерируется.");
					}
					else {
						event.reply("Пользователь не модерируется.");
					}
					

				}
				else if (type == "add_nomod_user") {
					dpp::snowflake user = std::get<dpp::snowflake>(event.get_parameter("user"));
					Guild& g = fm.get_guild(event.command.guild_id);
					User* u = g.get_user(user);
					if (u->is_moderate_text_enable()) {
						u->moderate_text_change(false);
						event.reply("Пользователь больше не модерируется.");
					}
					else {
						event.reply("Ничего не изменилось, пользователь уже не модерирован.");
					}
				}
				else if (type == "remove_nomod_user") {
					dpp::snowflake user = std::get<dpp::snowflake>(event.get_parameter("user"));
					Guild& g = fm.get_guild(event.command.guild_id);
					User* u = g.get_user(user);
					if (!u->is_moderate_text_enable()) {
						u->moderate_text_change(true);
						event.reply("Пользователь теперь модерируется.");
					}
					else {
						event.reply("Ничего не изменилось, пользователь уже модеруется.");
					}
				}
		}
		else {
			event.reply(
				dpp::message(to_utf8(L"У вас нет прав на это действие")).set_flags(dpp::m_ephemeral)
			);
		}
		};

	handlers_cmd["warn"] = [&](const dpp::slashcommand_t& event) {
		
		Guild gl;
		gl = fm.get_guild_r(event.command.guild_id);
		User* uu = gl.get_user(event.command.usr.id);
		if (uu->is_admin()) {
			std::string text = std::get<std::string>(event.get_parameter("reason"));
			dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("user"));
			Guild& g = fm.get_guild(event.command.guild_id);
			User* u;
			u = g.get_user(user_id);
			u->add_warn(text);
			std::string name = event.command.guild_id.str();
			auto guild = event.command.get_guild();
			std::string reply = to_utf8(L"# Вы получили варн на сервере: ") + guild.name + to_utf8(L"\n С причиной: ") + text + to_utf8(L"\n\n  **Постарайтесь больше не нарушать правила сервера!**");
			bot.direct_message_create(user_id, dpp::message(reply));
			event.reply(to_utf8(L"**Пользователь получил предупреждение!**"));
		}
		else {
			event.reply(
				dpp::message(to_utf8(L"У вас нет прав на это действие")).set_flags(dpp::m_ephemeral)
			);
		}
		};

	handlers_cmd["warn_remove"] = [&](const dpp::slashcommand_t& event) {

		Guild gl;
		gl = fm.get_guild_r(event.command.guild_id);
		User* uu = gl.get_user(event.command.usr.id);
		if (uu->is_admin()) {
			dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("user"));
			dpp::snowflake warn_id = std::get<std::string>(event.get_parameter("warn_id"));
			if (warn_id != 0) {
				Guild& g = fm.get_guild(event.command.guild_id);
				User* u;
				u = g.get_user(user_id);
				std::cout << warn_id << "\n";
				u->remove_warn(warn_id);
				event.reply(to_utf8(L"Сняла варн пользователю!"));
			}
			else {
				event.reply(to_utf8(L"У пользователя нет варнов!"));
			}
		}
		else {
			event.reply(
				dpp::message(to_utf8(L"У вас нет прав на это действие")).set_flags(dpp::m_ephemeral)
			);
		}
		};

	handlers_cmd["warn_check"] = [&](const dpp::slashcommand_t& event) {

		Guild gl;
		gl = fm.get_guild_r(event.command.guild_id);
		User* uu = gl.get_user(event.command.usr.id);
		if (uu->is_admin()) {
			dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("user"));
			uu = gl.get_user(user_id);
			std::unordered_map<int, std::string>warns = uu->get_warns();
			std::string reply = to_utf8(L"**Предупреждения пользователя:** <@") + std::to_string(uu->get_user_id()) + ">\n";
			for (const auto& [id, warn] : warns) {
				reply = reply + to_utf8(L"> Предупреждение #") + std::to_string(id) + to_utf8(L" Причина: ") + warn + "\n";
			}
			if (!warns.empty()) {
				event.reply(reply);
			}else{
					event.reply(to_utf8(L"У пользователя нет предупреждений"));
			}
		}
		else {
			event.reply(
				dpp::message(to_utf8(L"У вас нет прав на это действие")).set_flags(dpp::m_ephemeral)
			);
		}
		};

	handlers_cmd["admin_list"] = [&](const dpp::slashcommand_t& event) {
		Guild g;
		g = fm.get_guild_r(event.command.guild_id);
		std::string reply = to_utf8(L"# Администрация этого сервера:\n");
		for (auto it : g.get_admin_ids()) {
			reply = reply + "> <@" + std::to_string(it) + ">\n";
		}
		event.reply(reply);
		};

	handlers_cmd["remove_admin"] = [&](const dpp::slashcommand_t& event) {
		dpp::snowflake id = std::get<dpp::snowflake>(event.get_parameter("user"));
		Guild gl;
		gl = fm.get_guild_r(event.command.guild_id);
		User* uu = gl.get_user(event.command.usr.id);
		if (uu->is_admin()) {
			Guild& g = fm.get_guild(event.command.guild_id);
			User* u = g.get_user(id);
			g.remove_admins_id(id);
			u->Remove_admin();
			event.reply(to_utf8(L"Удалила пользователя с админов: <@") + std::to_string(id) + ">");
		}
		else {
			event.reply(
				dpp::message(to_utf8(L"У вас нет прав на это действие")).set_flags(dpp::m_ephemeral)
			);
		}
		};

	handlers_cmd["add_admin"] = [&](const dpp::slashcommand_t& event) {
		dpp::snowflake id = std::get<dpp::snowflake>(event.get_parameter("user"));
		Guild gl;
		gl = fm.get_guild_r(event.command.guild_id);
		User* uu = gl.get_user(event.command.usr.id);
		if (uu->is_admin()) {
		Guild& g = fm.get_guild(event.command.guild_id);
		User* u = g.get_user(id);
		g.add_admins_id(id);
		u->Add_admin();
		event.reply(to_utf8(L"Добавила пользователя в админы: <@") + std::to_string(id) + ">");
		}
		else {
			event.reply(
				dpp::message(to_utf8(L"У вас нет прав на это действие")).set_flags(dpp::m_ephemeral)
			);
		}
		};

	handlers_cmd["remove_role"] = [&](const dpp::slashcommand_t& event) {
		Guild gl;
		gl = fm.get_guild_r(event.command.guild_id);
		User* u = gl.get_user(event.command.usr.id);
		if (u->is_admin()) {
			dpp::snowflake role_id = std::get<dpp::snowflake>(event.get_parameter("role"));
			Guild& g = fm.get_guild(event.command.guild_id);
			g.remove_lvl_role(role_id);
			event.reply(to_utf8(L"Удалила роль!"));
		}
		else {
			event.reply(
				dpp::message(to_utf8(L"У вас нет прав на это действие")).set_flags(dpp::m_ephemeral)
			);
		}
		};

	handlers_cmd["add_role_exp"] = [&](const dpp::slashcommand_t& event) {
		Guild gl;
		gl = fm.get_guild_r(event.command.guild_id);
		User* u = gl.get_user(event.command.usr.id);
		if (u->is_admin()) {
			dpp::snowflake role_id = std::get<dpp::snowflake>(event.get_parameter("role"));
			int exp = static_cast<int>(std::get<int64_t>(event.get_parameter("exp")));
			std::string type = std::get<std::string>(event.get_parameter("type"));
			Guild& g = fm.get_guild(event.command.guild_id);
			g.add_lvl_role(role_id, exp, type);
			event.reply(to_utf8(L"Добавила роль!"));
		}
		else {
			event.reply(
				dpp::message(to_utf8(L"У вас нет прав на это действие")).set_flags(dpp::m_ephemeral)
			);
		}
		};

	handlers_cmd["role_list"] = [&](const dpp::slashcommand_t& event) {
		Guild g;
		g = fm.get_guild_r(event.command.guild_id);
		User* u = g.get_user(event.command.usr.id);
		if (u->is_admin()) {
			std::string reply = to_utf8(L"## Текущии роли: \n");
			for (auto it : g.get_lvl_roles()) {
				reply = reply + "> <@&" + std::to_string(it.id_role) + to_utf8(L"> Опыт: ") + std::to_string(it.xp_role) + to_utf8(L" Тип выдачи: ") + it.type + "\n";
			}
			event.reply(reply);
		}
		else {
			event.reply(
				dpp::message(to_utf8(L"У вас нет прав на это действие")).set_flags(dpp::m_ephemeral)
			);
		}
		};

	handlers_cmd["leaderscore"] = [&](const dpp::slashcommand_t& event) {
		std::string type = std::get<std::string>(event.get_parameter("type"));
		std::string large = std::get<std::string>(event.get_parameter("large"));
		std::string reply;
		int lenght = std::stoi(large);
		if (type == "xp_text") {
			std::vector<std::pair<dpp::snowflake, int>> users;
			Guild g;
			g = fm.get_guild_r(event.command.guild_id);
			for (auto& [it, u] : g.get_users()) {
				users.push_back({ u.get_user_id(), u.get_user_exp_text() });
			}
			std::sort(users.begin(), users.end(), [](auto& a, auto& b) {
				return a.second > b.second;
				});
			int it = 0;
			reply = to_utf8(L"## Лидеры по опыту в чате : \n");
			for (auto [user, exp] : users) {
				if (lenght <= it) {
					break;
				}
				it++;
				reply = reply + "> " + std::to_string(it) + "# <@" + std::to_string(user) + "> " + to_utf8(L" **Опыт:** ") + std::to_string(exp) + "\n";
				
			}
		}
		else if (type == "xp_voice") {
			std::vector<std::pair<dpp::snowflake, int>> users;
			Guild g;
			g = fm.get_guild_r(event.command.guild_id);
			for (auto& [it, u] : g.get_users()) {
				users.push_back({ u.get_user_id(), u.get_user_exp_voice() });
			}
			std::sort(users.begin(), users.end(), [](auto& a, auto& b) {
				return a.second > b.second;
				});
			int it = 0;
			reply = to_utf8(L"## Лидеры по опыту в голосе : \n");
			for (auto [user, exp] : users) {
				if (lenght <= it) {
					break;
				}
				it++;
				float expf = static_cast<float>(exp);
				float hours = std::round(expf * 15 / 3600 * 10.0f) / 10.0f;

				std::ostringstream ss;
				ss << std::fixed << std::setprecision(1) << hours;

				reply = reply + "> " + std::to_string(it) + "# <@" + std::to_string(user) + "> "
					+ to_utf8(L" **Опыт:** ") + std::to_string(exp) + to_utf8(L"   :microphone: ")
					+ ss.str() + to_utf8(L"**ч** \n");
			}
		}
		else if (type == "mute_time") {
			std::vector<std::pair<dpp::snowflake, int>> users;
			Guild g;
			g = fm.get_guild_r(event.command.guild_id);
			for (auto& [it, u] : g.get_users()) {
				users.push_back({ u.get_user_id(), u.get_time_muted() });
			}
			std::sort(users.begin(), users.end(), [](auto& a, auto& b) {
				return a.second > b.second;
				});
			int it = 0;
			reply = to_utf8(L"## Лидеры по муту в голосе : \n");
			for (auto [user, exp] : users) {
				if (lenght <= it) {
					break;
				}
				it++;
				float expf = static_cast<float>(exp);
				float hours = std::round(expf * 15 / 3600 * 10.0f) / 10.0f;

				std::ostringstream ss;
				ss << std::fixed << std::setprecision(1) << hours; 

				reply = reply + "> " + std::to_string(it) + "# <@" + std::to_string(user) + "> "
					+ to_utf8(L" **Опыт:** ") + std::to_string(exp) + to_utf8(L"   :microphone: ")
					+ ss.str() + to_utf8(L"**ч** \n");
			}
		}else if (type == "xp_swear") {
			std::vector<std::pair<dpp::snowflake, int>> users;
			Guild g;
			g = fm.get_guild_r(event.command.guild_id);
			for (auto& [it, u] : g.get_users()) {
				users.push_back({ u.get_user_id(), u.get_user_exp_swears() });
			}
			std::sort(users.begin(), users.end(), [](auto& a, auto& b) {
				return a.second > b.second;
				});
			int it = 0;
			reply = to_utf8(L"## Лидеры по матершинице в чате : \n");
			for (auto [user, exp] : users) {
				if (lenght <= it) {
					break;
				}
				it++;
				reply = reply + "> " + std::to_string(it) + "# <@" + std::to_string(user) + "> " + to_utf8(L" **Опыт:** ") + std::to_string(exp) + "\n";

			}
		}
		
		event.reply(reply);
		};
}
int main()
{

	SetConsoleOutputCP(CP_UTF8);
	setlocale(LC_ALL, "ru_RU.UTF-8");
	std::string token;
	std::ifstream filet("D:\\DEV\\Disbot\\token.txt");
	filet >> token;
	filet.close();

	dpp::cluster bot(token, dpp::i_default_intents | dpp::i_message_content);
	dpp::snowflake owner_id = 879386342931451914;
	load_commads(bot);
	load_swears("D:\\DEV\\Disbot\\swears.txt");
	bot_thread(bot);
	bot_thread_saver(bot);
	bot_thread_exp_lvls(bot);

	bot.on_log(dpp::utility::cout_logger());

	bot.on_ready([&bot](const dpp::ready_t& event) {
		std::cout << "\n\nver 1.0 \n\n";
		std::cout << "DPP version: " << dpp::utility::version() << std::endl;
		if (fm.load_guilds("D:\\DEV\\Disbot\\tests")) {
			std::cout << "Loading guilds: ";
			SetColor(2);
			std::cout << "Sucessfuly\n";
			SetColor();
		}
		else {
			SetColor(4);
			std::cout << "Loading guilds: ";
			std::cout << "failed\n";
			SetColor();
		}
		
		
		
	});

	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		std::string name = event.command.get_command_name();
		Guild& g = fm.get_guild(event.command.guild_id);

		std::string log_path = "D:\\DEV\\Disbot\\tests\\" + std::to_string(g.get_id()) + ".txt";
		std::ofstream file(log_path, std::ios::app);
		if (file.is_open()) {
			file << getCurrentDateTime() << " " << event.command.msg.author.id << ": " << event.command.msg.content << " " << event.command.channel_id << std::endl;
			std::cout << getCurrentDateTime() << " " << event.command.msg.author.id << ": " << event.command.msg.content << " " << event.command.channel_id << std::endl;
			file.close();
		}

		if (handlers_cmd.contains(name)) {
			handlers_cmd[name](event);
		}
		else {
			event.reply(to_utf8(L"Команда не реализована."));
		}
		});


	bot.on_autocomplete([&](const dpp::autocomplete_t& event) {
		if (event.name == "warn_remove") {
			

			
			
			auto& opts = event.options;
			dpp::snowflake user_id;
			bool have_user = false;
			for (auto& opt : opts) {
				if (opt.name == "user" && opt.type == dpp::co_user) {
					have_user = true;
					user_id = std::get<dpp::snowflake>(opt.value);
					break;
				}
			}
			if (!have_user) {
				return;
			}

			
			Guild& g = fm.get_guild_r(event.command.guild_id);

			User* u = g.get_user(user_id);
			auto warns = u->get_warns();

			dpp::interaction_response resp(dpp::ir_autocomplete_reply);
			size_t it = 0;
			for (const auto& [id, reason] : warns) {
				size_t size = 0;
				if (reason.size() < 120) {
					size = reason.size();
				} else size = 120;
				resp.add_autocomplete_choice(dpp::command_option_choice("#" + std::to_string(id) + " " + reason.substr(0, size), std::to_string(id)));
				it++;
				if (it >= 25) break;
			}
			if (warns.empty()) {
				resp.add_autocomplete_choice(dpp::command_option_choice(to_utf8(L"Варны отсутствуют."), std::to_string(0)));
			}
			bot.interaction_response_create(
				event.command.id,
				event.command.token,
				resp
			);
			return;
		}
		});

	bot.on_guild_create([&bot](const dpp::guild_create_t& event) {
		std::this_thread::sleep_for(std::chrono::seconds(5));

		dpp::guild g = event.created;
		if (!fm.find_guild(g.id)) {
			if (dpp::run_once<struct cmd_reg>()) {
				/*auto cmds = build_commands(bot.me.id);
				for (auto& cmd : cmds) {
					std::this_thread::sleep_for(std::chrono::milliseconds(250));
					bot.guild_command_create(cmd, g.id);
				}*/
				Guild gl = fm.get_guild(g.id);
				User u;
				u.Create_user(g.owner_id, 0, 0, 0, {}, false, true);
				gl.add_user(u);

				SetColor(12);
				std::cout << "Bot joined guild: "
					<< g.name << " (" << g.id << ")\n";
				SetColor(7);
			}
		}

		});

	bot.on_voice_state_update([&bot](const dpp::voice_state_update_t& event) {
		const auto& vs = event.state;
		
		if (vs.channel_id == 0) {
			if (v.is_in_voice_here(vs.guild_id)) {
				std::unordered_map <dpp::snowflake, dpp::snowflake> voices = v.check_voices();
				dpp::guild* g = dpp::find_guild(vs.guild_id);
				dpp::snowflake guild_id = vs.guild_id;
				const auto& vs = g->voice_members;
				bool finded = false;
				for (const auto& [id, state] : vs) {
					if (voices[guild_id] == state.channel_id && state.user_id != bot.me.id) {
						finded = true;
					}
				}
				if (!finded) {
					SetColor(5);
					std::cout << "No users with bot leave.\n";
					v.leave_voice(event, guild_id);
					SetColor();
				}
			}
		}

		else {
			if (v.is_in_voice_here(vs.guild_id)) {
				std::unordered_map <dpp::snowflake, dpp::snowflake> voices = v.check_voices();
				dpp::guild* g = dpp::find_guild(vs.guild_id);
				dpp::snowflake guild_id = vs.guild_id;
				const auto& vs = g->voice_members;
				bool finded = false;
				for (const auto& [id, state] : vs) {
					if (voices[guild_id] == state.channel_id && state.user_id != bot.me.id) {
						finded = true;
					}
				}
				if (!finded) {
					SetColor(5);
					std::cout << "No users with bot leave.\n";
					v.leave_voice(event, guild_id);
					SetColor();
				}
			}
		}
		});


	bot.on_message_create([&bot, owner_id](const dpp::message_create_t& event) {
		dpp::snowflake message_id = event.msg.id;
		dpp::snowflake author_id = event.msg.author.id;
		dpp::snowflake channel_id = event.msg.channel_id;
		dpp::snowflake guild_id = event.msg.guild_id;
		std::string message = to_utf8(string_to_wstring(event.msg.content));
		std::string lmessage = to_lower_utf8(message);
		Guild& g = fm.get_guild(guild_id);
		
		std::string log_path = "D:\\DEV\\Disbot\\tests\\" + std::to_string(g.get_id()) + ".txt";
		std::ofstream file(log_path, std::ios::app);
		if (file.is_open()) {
			file << getCurrentDateTime() << " " << author_id << ": " << message << " " << channel_id << std::endl;
			std::cout << getCurrentDateTime() << " " << author_id << ": " << message << " " << channel_id << std::endl;
			file.close();
		}
		if (!g.is_banned_channel(channel_id)) {
			for (const auto& word : split(lmessage)) {
				if (g.has_banned_word(normalize(word))) {
					bot.message_delete(message_id, channel_id);
					break;
				}
				if (swears.contains(normalize(word))) {
					Guild& guild = fm.get_guild(guild_id);
					User* u = guild.get_user(author_id);
					if (guild.is_anti_swear()) {
						u->Add_exp_swears(1);
						if (u->is_moderate_text_enable()) {
							bot.message_delete(message_id, channel_id);
						}
					}
					else {
						u->Add_exp_swears(1);
					}
				}
			}
		}

		if (!g.has_user(author_id)) {
			std::cout << "Added new exp user: " << author_id << " Guild: " << guild_id << "\n";
			User u;
			dpp::guild* guild = dpp::find_guild(guild_id);
			if (guild_id != 0  && author_id == guild->owner_id) {
				u.Create_user(author_id, 0,0,0, {}, false, true);
				g.add_user(u);
			}
			else {
				u.Create_user(author_id);
				g.add_user(u);
			}
			return;
		}else {
			User* u = g.get_user(author_id);
			u->Add_exp_text(1);
		}
		for (auto it : g.get_users()) {
			auto s = it.second;
		}

		// TTS messages
		User* u = g.get_user(author_id);
		if (u->is_tts_enable()) {
			if (v.is_in_voice_here(guild_id)) {
				v.play(v.tts_create(message, u, std::to_string(guild_id), "D:\\DEV\\Disbot\\tts\\"), guild_id, event);
			}
		}

		if (message.substr(0, message.find(" ")) == "play" && author_id == owner_id) {
			std::vector <std::string> splited = split(message);
			event.reply("trying to load " + splited[1]);
			v.play(splited[1], guild_id, event);
		}
		if (message.substr(0, message.find(" ")) == "join") {
			std::vector <std::string> splited = split(message);
			if (author_id == owner_id) {
				if (splited.size() > 1) {
					if (v.join_voice(std::stoull(splited[1]), event, guild_id)) {
						bot.message_add_reaction(event.msg.id, channel_id, "🟢");
						

					}
					else {
						bot.message_add_reaction(event.msg.id, channel_id, "🔴");
					}
				}
				else {
					if (v.join_voice(author_id, event, guild_id)) {
						event.reply(dpp::message(to_utf8(L"Зашла в войс!")).set_flags(dpp::m_ephemeral));

					}
					else {
						event.reply(dpp::message(to_utf8(L"Не могу зайти в войс!")).set_flags(dpp::m_ephemeral));
					}
				}
			}
			else {
				if (v.join_voice(author_id, event, guild_id)) {
					event.reply(dpp::message(to_utf8(L"Зашла в войс")).set_flags(dpp::m_ephemeral));
				}
				else {
					event.reply(dpp::message(to_utf8(L"Не могу зайти в войс")).set_flags(dpp::m_ephemeral));
				}
			}
		}
		if (message == "leave") {

			if (v.leave_voice(event, guild_id)) {
				event.reply(dpp::message(to_utf8(L"Вышла из войса")).set_flags(dpp::m_ephemeral));
			}
			else {
				event.reply(dpp::message(to_utf8(L"Не могу выйти из войса")).set_flags(dpp::m_ephemeral));
			}
		}
		if (message == "voice list" && author_id == owner_id) {
			std::string reply;
			for (const auto& [guild, channel] : v.check_voices()) {
				reply = reply + " g: " + std::to_string(guild) + "  ch: " + std::to_string(channel) + "\n";
			}
			event.reply("Voice connected:\n" + reply);
		}
		if (message.substr(0, message.find(" ")) == "tts" && author_id == owner_id) {
			Guild& guild = fm.get_guild_r(guild_id);
			v.play(v.tts_create(message, guild.get_user(author_id), std::to_string(guild_id), "D:\\DEV\\Disbot\\tts\\"), guild_id, event);
		}


		if (message == "save" && author_id == owner_id) {
			if (fm.save_guilds("D:\\DEV\\Disbot\\tests\\")) {
				event.reply(to_utf8(L"Успешно сохранила дату."));
			}
			else {
				event.reply(to_utf8(L"Ошибка сохранения даты."));
			}
		}
		if (message == "load" && author_id == owner_id) {
			if (fm.load_guilds("D:\\DEV\\Disbot\\tests")) {
				event.reply(to_utf8(L"Успешно загрузила дату."));
			}
			else {
				event.reply(to_utf8(L"Ошибка загрузки даты."));
			}
		}
		if (message == "exp_v" && author_id == owner_id) {
			for (auto [gl_id, gl] : fm.get_guilds()) {
				std::cout << "Guild: " << gl.get_id() << "\n";
				for (auto [u_id,u] : gl.get_users()) {
					std::cout << "User: " << u.get_user_id() << " User_exp_voice: " << u.get_user_exp_voice() << "\n";
				}
			}
		}
		if (message.substr(0, message.find(" ")) == "com_reg" && author_id == owner_id) {
			if (message == "com_reg delete servers here") {
				bot.guild_bulk_command_delete(guild_id);
				event.reply("commands are deleted here");
			}
			else if (message == "com_reg delete") {
				bot.global_bulk_command_delete();
				event.reply("commands are deleted");
			}
			if (message == "com_reg delete servers") {
				for (auto it : fm.get_guilds_r()) {
					std::this_thread::sleep_for(std::chrono::milliseconds(250));
					bot.guild_bulk_command_delete(it.first);
				}
				event.reply("commands are deleted");
			}
			else {
				if (message == "com_reg here") {
					if (dpp::run_once<struct cmd_reg>()) {

						auto cmds = build_commands(bot.me.id);
						std::string reply = "**commands registred here:**\n";
						for (auto& cmd : cmds) {
							std::this_thread::sleep_for(std::chrono::milliseconds(250));
							reply = reply + cmd.name + "\n";
							bot.guild_command_create(cmd, guild_id);
						}

						event.reply(reply);
					}
				}
				else {
					if (dpp::run_once<struct cmd_reg>()) {

						auto cmds = build_commands(bot.me.id);
						for (auto it : fm.get_guilds_r()) {
							for (auto& cmd : cmds) {
								std::this_thread::sleep_for(std::chrono::milliseconds(250));
								bot.guild_command_create(cmd, it.first);
							}
						}
						event.reply("command are registred");
					}
				}
			}
		}
		if (message == "global command reg" && author_id == owner_id) {
			if (dpp::run_once<struct cmd_reg>()) {

				auto cmds = build_commands(bot.me.id);
				std::string reply = "**commands registred here:**\n";
					for (auto& cmd : cmds) {
						reply = reply + cmd.name + "\n";
						std::this_thread::sleep_for(std::chrono::milliseconds(1000));
						bot.global_command_create(cmd);
					}
				
				event.reply(reply);
			}
		}
		if (lmessage.substr(0, message.find(" ")) == "guild_name" && author_id == owner_id) {
			std::vector<std::string> splited = split(message);
			dpp::snowflake id = stoull(splited[1]);
			dpp::guild* g = dpp::find_guild(id);
			std::string name = g->name;
			event.reply("Guild name: " + name);
		}
	});

	bot.start(dpp::st_wait);
}
