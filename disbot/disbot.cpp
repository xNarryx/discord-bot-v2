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
#include <curl/curl.h>
#include <regex>
#include "FileManager.h"
#include "guild.h"
#include "User.h"
#include "VoiceManager.h"


file_manager fm;
VoiceManager v;
std::unordered_map<std::string, std::function<void(const dpp::slashcommand_t&)>> handlers_cmd;
std::unordered_set<std::string> swears;
bool has_swear(std::string str) {
	for (auto& word : swears) {
		
		if (str.find(word) != std::string::npos) {
			return(true);
		}
	}
	return(false);
}
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
std::vector<std::string> utf8_split(const std::string& str)
{
	std::vector<std::string> result;

	for (size_t i = 0; i < str.size();)
	{
		unsigned char c = str[i];

		size_t len = 1;

		if ((c & 0b10000000) == 0) len = 1;
		else if ((c & 0b11100000) == 0b11000000) len = 2;
		else if ((c & 0b11110000) == 0b11100000) len = 3;
		else if ((c & 0b11111000) == 0b11110000) len = 4;

		result.push_back(str.substr(i, len));
		i += len;
	}

	return result;
}
std::string normalize(const std::string str) {
	std::unordered_set<std::string> chars = {
		".", ",", ";", "'", "[", "]", "-", "=", "_", "+",
		"!", "@", "#", "$", "%", "^", "&", "?", "*",
		"(", ")", "~", "`", ":", "<", ">", "\\", "|", "/", "{", "}"
	};

	std::unordered_map<std::string, std::string> letters = {
		{"0", "о"},
		{"4", "е"},
		{"e", "е"},
		{"l", "л"},
		{"a", "а"},
		{"o", "о"},
		{"y", "у"},
		{"i", "й"},
		{"x", "х"},
		{"p", "п"},
		{"u", "у"},
		{"7", "ч"}
	};


	std::string string, prevch = "";
	auto sybmols = utf8_split(str);
	for (auto& ch : sybmols) {
		if (chars.contains(ch) || ch == prevch)
			continue;
		if (letters.contains(ch)) {
			string += letters[ch];
		}
		else {
			string += ch;
		}
		prevch = ch;
	}
	return string;
}
std::string extract_digits(const std::string& str)
{
	std::string result;

	for (char c : str)
	{
		if (std::isdigit(static_cast<unsigned char>(c)))
		{
			result += c;
		}
	}

	return result;
}
std::string delete_https(const std::string str) {
	std::regex link_pattern("https?://[^\\s]+");
	std::string cleaned = std::regex_replace(str, link_pattern, "");
	link_pattern = ("<:?[^\\s]+");
	cleaned = std::regex_replace(cleaned, link_pattern, "");
	return cleaned;
}
std::string replace_user_id_on_it_name(const std::string str, dpp::snowflake guild_id) {
	dpp::guild_member gm;
	std::string name;
	dpp::snowflake user_id;
	std::vector<std::string> args = split(str);
	for (auto& arg : args) {
		try {
			if (arg.substr(0, 2) == "<@") {
				user_id = std::stoull(extract_digits(arg));
				gm.guild_id = guild_id;
				gm.user_id = user_id;
				if (auto* user = gm.get_user())
				{
					name = user->username;
					arg = name;
				}
			}
		}
		catch (...) {

		}
	}
	std::string result;
	for (auto& arg : args) {
		result += arg + " ";
	}
	return result;
}
// gemini
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}
std::string ask_gemini(const std::string prompt, std::string system_promp, std::string model, std::string token)
{
	CURL* curl = curl_easy_init();
	if (!curl)
		return "Failed to init curl";

	std::string response;
	std::string system_prompt =
		"Ты — девушка по имени Nyphomania. "
		"у тебя есть доступ к истории местного чата для контекста "
		"тебе не надо отвечать или говорить извинения за то что ты не овтетила "
		"Не представляйся и не приветствуй пользователя без необходимости. "
		"это чат где много людей, старайся быть собой "
		"Отвечай кратко (до 150 слов). "
		"Не повторяйся и не объясняй свою роль, если не спрашивают. "
		"Будь пожалуйста чуть дружелюбной, но не слишком, не оскорбляй всех без причины, старайся поддержать дружелюбность "
		"история чата идет сверху вниз, старайся запоминать кто что написал, но не упоминать это в каждом ответе. " + system_promp;
	std::string api_key = token;

	std::string url =
		"https://generativelanguage.googleapis.com/v1beta/models/" +
		model + ":generateContent?key=" + api_key;
	
	nlohmann::json body;

	body["contents"] = {
	{
		{
			"parts",
			{
				{{"text", system_prompt + prompt}}
			}
		}
	}
	};
	std::cout << system_prompt << "\n\nПользователь: " << prompt << "\n";
	std::string json_body = body.dump();

	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

	CURLcode res = curl_easy_perform(curl);

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK)
		return curl_easy_strerror(res);

	return response;
}
std::string get_answer(const std::string prompt, std::string system_prompt, std::string token)
{
	std::vector<std::string> models = {
		"gemini-3.1-flash-lite",
		"gemini-2.5-flash-lite",
		"gemini-2.5-flash",
		"Gemma 4 31B"
	};

	bool has_answer = false;
	nlohmann::json json;
	for (auto& model : models)
	{
		std::string raww = ask_gemini(prompt, system_prompt, model, token);
		json = nlohmann::json::parse(raww);

		std::cout << json << std::endl;

		if (json.contains("candidates") &&
			!json["candidates"].empty())
		{
			has_answer = true;
			break;
		}
	}

	if (has_answer) {
		if (json.contains("error"))
			return json["error"]["message"].get<std::string>();

		return json["candidates"][0]
			["content"]["parts"][0]["text"]
			.get<std::string>();
	}

	return "Извини, я пока не могу тебе ответить.";
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
					try {
						if (dpp::find_guild(g.get_id())) {
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
					catch (...) {
						SetColor(12);
						dpp::snowflake id = g.get_id();
						std::cout << "Невозможно проверить гильдию. Возможен кик. Удаляю из сохранения временно гильдию: " << id;
						SetColor();
						fm.delete_guild_for_now(id);
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
				try {
					if (dpp::guild* g_l = dpp::find_guild(g.get_id())) {
						for (auto& [user_id, u] : g.get_users()) {
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
				catch (...) {
					SetColor(12);
					dpp::snowflake id = g.get_id();
					std::cout << "Невозможно проверить гильдию. Удаляю из сохранения временно гильдию: " << id;
					SetColor();
					fm.delete_guild_for_now(id);
				}
			}
		}
		}).detach();
}

std::vector<dpp::slashcommand> build_commands(dpp::snowflake bot_id) { // commands for global
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
	warn.add_option(
		dpp::command_option(
			dpp::co_boolean,
			"ping_user",
			to_utf8(L"Уведомить пользователя об этом?"),
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
#pragma region auto_reply
	dpp::slashcommand auto_reply(
		"auto_reply",
		to_utf8(L"Добавить или убрать авто ответ для пользователя."),
		bot_id
	);
	auto_reply.add_option(
		dpp::command_option(dpp::co_string, "type", "Выберите вариант!", true)
		.add_choice(dpp::command_option_choice(to_utf8(L"Добавить авто-ответ"), "add_reply"))
		.add_choice(dpp::command_option_choice(to_utf8(L"убрать авто-ответ"), "remove_reply"))
	);
	auto_reply.add_option(
		dpp::command_option(dpp::co_string, "help-delete", "ПОМОЩЬ! И Удаление уже существующих автоматических ответов пользователям.", false)
		.set_auto_complete(true)
	);
	auto_reply.add_option(
		dpp::command_option(dpp::co_string, "key_word", "Обязательно: Введите ключеное слово на которое будет ответ", false)
	);
	auto_reply.add_option(
		dpp::command_option(dpp::co_string, "message", "Обязательно: Введите которое будет отсылаться пользователю", false)
	);
	auto_reply.add_option(
		dpp::command_option(dpp::co_channel, "channel", "Обязательно: Введите канал где будет работать ответ", false)
	);

	cmds.push_back(auto_reply);
#pragma endregion
#pragma region report_issue
	dpp::slashcommand report_issue(
		"report_issue",
		to_utf8(L"Зарепортить проблему в боте"),
		bot_id
	);
	report_issue.add_option(
		dpp::command_option(
			dpp::co_string,
			"text",
			to_utf8(L"Введите сообщение"),
			false
		)
	);
	cmds.push_back(report_issue);
#pragma endregion
#pragma region AI_answers
	dpp::slashcommand ai_answers(
		"ai_answers",
		to_utf8(L"Редактировать память, и добавить базовый промпт для канала"),
		bot_id
	);
	ai_answers.add_option(
		dpp::command_option(
			dpp::co_string,
			"type",
			to_utf8(L"Выберите действие"),
			true
		)
		.add_choice(dpp::command_option_choice(to_utf8(L"Очистить память"), "clear_memory"))
		.add_choice(dpp::command_option_choice(to_utf8(L"Добавить базовый промпт для канала"), "add_base_prompt"))
	);
	ai_answers.add_option(
		dpp::command_option(
			dpp::co_string,
			"text",
			to_utf8(L"Текст для базового промпта  \"-\" - для очистки промпта"),
			false
		)
	);
	ai_answers.add_option(
		dpp::command_option(
			dpp::co_channel,
			"channel",
			to_utf8(L"канал для промпта"),
			false
		)
	);

	cmds.push_back(ai_answers);
#pragma endregion
	return cmds;
}
std::vector<dpp::slashcommand> build_commands_local(dpp::snowflake bot_id) { // commands for local use
	std::vector<dpp::slashcommand> cmds;

#pragma region auto_reply
	dpp::slashcommand auto_reply(
		"auto_reply",
		to_utf8(L"Добавить или убрать авто ответ для пользователя."),
		bot_id
	);
	auto_reply.add_option(
		dpp::command_option(dpp::co_string, "type", "Выберите вариант!", true)
		.add_choice(dpp::command_option_choice(to_utf8(L"Добавить авто-ответ"), "add_reply"))
		.add_choice(dpp::command_option_choice(to_utf8(L"убрать авто-ответ"), "remove_reply"))
	);
	auto_reply.add_option(
		dpp::command_option(dpp::co_string, "help-delete", "ПОМОЩЬ! И Удаление уже существующих автоматических ответов пользователям.", false)
		.set_auto_complete(true)
	);
	auto_reply.add_option(
		dpp::command_option(dpp::co_string, "key_word", "Обязательно: Введите ключеное слово на которое будет ответ", false)
	);
	auto_reply.add_option(
		dpp::command_option(dpp::co_string, "message", "Обязательно: Введите которое будет отсылаться пользователю", false)
	);
	auto_reply.add_option(
		dpp::command_option(dpp::co_channel, "channel", "Обязательно: Введите канал где будет работать ответ", false)
	);
	
	cmds.push_back(auto_reply);
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
				text = std::get<std::string>(event.get_parameter("text"));
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

	handlers_cmd["ai_answers"] = [&](const dpp::slashcommand_t& event) {
		std::string text = " ";
		dpp::snowflake channel;
		Guild gl;
		gl = fm.get_guild_r(event.command.guild_id);
		User* uu = gl.get_user(event.command.usr.id);
		if (uu->is_admin()) {
			auto param = event.get_parameter("text");
			if (param.index() != 0) {
				text = std::get<std::string>(event.get_parameter("text"));
			}
			param = event.get_parameter("channel");
			if (param.index() != 0) {
				channel = std::get<dpp::snowflake>(event.get_parameter("channel"));
			}
			else {
				channel = event.command.channel_id;
			}

			auto type = std::get<std::string>(event.get_parameter("type"));
			if (type == "clear_memory") {
				Guild& g = fm.get_guild(event.command.guild_id);
				if (g.clean_channel_history(channel)) {
					event.reply(
						dpp::message(to_utf8(L"Удалила память канала.")).set_flags(dpp::m_ephemeral)
					);
				}
				
			} if (type == "add_base_prompt") {
				Guild& g = fm.get_guild(event.command.guild_id);
				if (g.get_channel_server_prompt(channel) != "") {
					g.add_channel_server_prompt(text, channel);
					event.reply(
						dpp::message(to_utf8(L"Изменила базовый промпт.")).set_flags(dpp::m_ephemeral)
					);
				}
				else {
					g.add_channel_server_prompt(text, channel);
					event.reply(
						dpp::message(to_utf8(L"Добавила базовый промпт.")).set_flags(dpp::m_ephemeral)
					);
				}
			}


		}
		else {
			event.reply(
				dpp::message(to_utf8(L"У вас нет прав на это действие")).set_flags(dpp::m_ephemeral)
			);
		}
		};

	handlers_cmd["report_issue"] = [&](const dpp::slashcommand_t& event) {
		std::string text = " ";
		
			auto param = event.get_parameter("text");
			if (param.index() != 0) {
				text = std::get<std::string>(event.get_parameter("text"));
			}
				bot.message_create(dpp::message(1519245595469156542, "User: <@" + std::to_string(event.command.usr.id) 
					+ "> Guild: " + std::to_string(event.command.guild_id) + " Reported text: " + text + "\n" 
					+ dpp::utility::user_mention(879386342931451914)).set_allowed_mentions(true));
				
				event.reply(
					dpp::message(to_utf8(L"Отправила ваш запрос.")).set_flags(dpp::m_ephemeral)
				);

	};

	handlers_cmd["auto_reply"] = [&](const dpp::slashcommand_t& event) {
		Guild gl;
		gl = fm.get_guild_r(event.command.guild_id);
		User* uu = gl.get_user(event.command.usr.id);
		if (uu->is_admin()) {
			std::string type = std::get<std::string>(event.get_parameter("type"));
			if (type == "add_reply") {
				std::string key_word = std::get<std::string>(event.get_parameter("key_word"));
				std::string message = std::get<std::string>(event.get_parameter("message"));
				Guild& g = fm.get_guild(event.command.guild_id);
				dpp::snowflake channel = std::get<dpp::snowflake>(event.get_parameter("channel"));
				g.add_auto_reply(key_word, message, channel);
				event.reply("Добавила авто ответ с **ключом**: " + key_word);
				
			}
			else if (type == "remove_reply") {
				std::string key_word = std::get<std::string>(event.get_parameter("help-delete"));
				Guild& g = fm.get_guild(event.command.guild_id);
				g.remove_auto_reply(key_word);
				event.reply("Удалила авто ответ с **ключом**: " + key_word);
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
			bool messaged = std::get<bool>(event.get_parameter("ping_user"));
			Guild& g = fm.get_guild(event.command.guild_id);
			User* u;
			u = g.get_user(user_id);
			u->add_warn(text);
			if (messaged) {
				std::string name = event.command.guild_id.str();
				auto guild = event.command.get_guild();
				std::string reply = to_utf8(L"# Вы получили варн на сервере: ") + guild.name + to_utf8(L"\n С причиной: ") + text;
				bot.direct_message_create(user_id, dpp::message(reply));
				event.reply(
					dpp::message(to_utf8(L"**Пользователь получил предупреждение с уведомлением!**")).set_flags(dpp::m_ephemeral)
				);
			}
			else {
				event.reply(
					dpp::message(to_utf8(L"**Пользователь получил предупреждение без уведомления!**")).set_flags(dpp::m_ephemeral)
				);
			}
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
			std::cout << warn_id << "\n";
			if (warn_id != 0) {
				Guild& g = fm.get_guild(event.command.guild_id);
				User* u;
				u = g.get_user(user_id);
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
	std::string token_gemini;
	std::ifstream filet1("D:\\DEV\\Disbot\\token_gemini.txt");
	filet1 >> token_gemini;
	filet1.close();

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
			}
			else size = 120;
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
	if (event.name == "auto_reply") {
		auto& opts = event.options;
		bool have_string = false;
		dpp::snowflake user_id = event.command.usr.id;
		std::string str;
		for (auto& opt : opts) {
			if (opt.name == "type" && opt.type == dpp::co_string) {
				have_string = true;
				str = std::get<std::string>(opt.value);
				break;
			}
		}
		if (!have_string) {
			return;
		}

		Guild& g = fm.get_guild_r(event.command.guild_id);

		User* u = g.get_user(user_id);
		dpp::interaction_response resp(dpp::ir_autocomplete_reply);
		if (u->is_admin()) {
			if (str == "add_reply") {
				resp.add_autocomplete_choice(dpp::command_option_choice(to_utf8(L"Напишите ключевое слово в key_word."), std::to_string(0)));
				resp.add_autocomplete_choice(dpp::command_option_choice(to_utf8(L"Напишите сообщение которое надо отправлять пользователю в message."), std::to_string(1)));
				resp.add_autocomplete_choice(dpp::command_option_choice(to_utf8(L"Напишите канал в котором будет отправка сообщения пользователям в channel."), std::to_string(2)));
				resp.add_autocomplete_choice(dpp::command_option_choice(to_utf8(L"(Бот будет проверять тоже только канал где выбран авто ответ для сообщения)"), std::to_string(2)));
			
			}
			else if (str == "remove_reply") {
				resp.add_autocomplete_choice(dpp::command_option_choice(to_utf8(L"Выберите вариант который надо удалить."), std::to_string(0)));
				struct AutoReplyData
				{
					std::string message;
					dpp::snowflake channel_id;
				};
				for (auto& [key, auto_rep] : g.get_auto_reply_messages()) {
					size_t size = 0;
					if (auto_rep.message.size() < 120) {
						size = auto_rep.message.size();
					}
					else size = 120;
					resp.add_autocomplete_choice(dpp::command_option_choice(to_utf8(string_to_wstring(auto_rep.message).substr(0, size)), key));
				}
			}


		}
		else {
			resp.add_autocomplete_choice(dpp::command_option_choice(to_utf8(L"У вас нет доступа."), std::to_string(0)));
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


	bot.on_message_create([&bot, owner_id, token_gemini](const dpp::message_create_t& event) {
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
				if (has_swear(normalize(word))) {
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
		if (g.is_banned_id(author_id)) {
			std::cout << "ignoring banned user";
			return;
		}
		for (auto it : g.get_users()) {
			auto s = it.second;
		}
		if (author_id != bot.me.id) {
			for (auto word : split(lmessage)) {
				if (g.is_auto_reply_word(word, channel_id)) {
					event.reply(g.get_auto_reply_message(word));
				}
			}
		}
		// history
		std::string history_message =
			delete_https(
				replace_user_id_on_it_name(
					"author <@" + std::to_string(author_id) + ">: " + message,
					guild_id
				)
			);

		if (history_message.size() > 300)
		{
			history_message = history_message.substr(0, 300);

			size_t last_space = history_message.rfind(' ');

			if (last_space != std::string::npos)
				history_message = history_message.substr(0, last_space);
			
		}
		g.add_message_history(history_message, channel_id);
		// gemini answers
		bot.message_get(event.msg.message_reference.message_id,
			event.msg.message_reference.channel_id,
			[&bot, channel_id, token_gemini, message, guild_id, &g, author_id]
			(const dpp::confirmation_callback_t& cc)
			{
				try {
					dpp::message msg = std::get<dpp::message>(cc.value);

					if (msg.author.id != bot.me.id)
						return;

					std::string result;
					for (auto& m : g.get_all_channel_history(channel_id))
						result += m + "\n";

					std::string sys_prompt = g.get_channel_server_prompt(channel_id) + " chat history: " + result;

					std::string prompt =
						"ответил на ваше сообщение Nyphomania:" + msg.content +
						" Пользователь написал: " +
						delete_https(replace_user_id_on_it_name(message, guild_id));

				
							try {
								auto answer = get_answer(replace_user_id_on_it_name("\nПользователь <@"+ std::to_string(author_id) + "> написал: " + prompt, guild_id), sys_prompt, token_gemini);

								bot.message_create(dpp::message(channel_id, answer));
							}
							catch (const std::exception& e) {
								std::cout << "AI error: " << e.what() << std::endl;
							}
				}
				catch (...) {
					std::cout << "message_get failed\n";
				}
			});
		if (lmessage.find("<@1276280240762523658>") != std::string::npos || lmessage.find("нимф") != std::string::npos) {
			std::string result;
			for (auto& message : g.get_all_channel_history(channel_id)) {
				result += message + "\n";
			}
			std::string sys_prompt = g.get_channel_server_prompt(channel_id) + "chat history: " + result;
			
			std::string prompt = delete_https(replace_user_id_on_it_name(message, guild_id));
			dpp::cluster* bot_ptr = &bot;

			std::thread([bot_ptr, prompt, channel_id, sys_prompt, token_gemini, author_id , guild_id]()
				{
					try
					{
						auto answer = get_answer(replace_user_id_on_it_name("\nПользователь <@" + std::to_string(author_id) + "> написал: " + prompt, guild_id), sys_prompt, token_gemini);

						bot_ptr->message_create(
							dpp::message(channel_id, answer)
						);
					}
					catch (const std::exception& e)
					{
						std::cout << "AI error: " << e.what() << std::endl;
					}
				}).detach();
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

		if (message.substr(0, message.find(" ")) == "ban_user" && author_id == owner_id) {
			dpp::snowflake user_id = std::stoull(extract_digits(message));
			for (auto& guilds : fm.get_guilds()) {
				guilds.second.add_banned_id(user_id);
				if (guilds.second.has_user(user_id)) {
					User* u = guilds.second.get_user(user_id);
					u->Add_ban();
				}
			}
			event.reply("user has banned in bot.");
		}
		if (message.substr(0, message.find(" ")) == "unban_user" && author_id == owner_id) {
			dpp::snowflake user_id = std::stoull(extract_digits(message));
			for (auto& guilds : fm.get_guilds()) {
				guilds.second.remove_banned_id(user_id);
				if (guilds.second.has_user(user_id)) {
					User* u = guilds.second.get_user(user_id);
					u->Remove_ban();
				}
			}
			event.reply("user has banned in bot.");
		}
		if (message == "history" && author_id == owner_id) {
			std::string reply = "### All history here:\n";
			for (auto& [channel, messages] : g.get_all_chat_history()) {
				reply += "\nChannel: " + std::to_string(channel);
				for (auto& message : messages.chat_history) {
					reply += "\n" + message;
				}
			}
			event.reply(reply);
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
			}else if (message == "com_reg local") {
					if (dpp::run_once<struct cmd_reg>()) {

						auto cmds = build_commands_local(bot.me.id);
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
				std::string reply = "**commands registred:**\n";
					for (auto& cmd : cmds) {
						reply = reply + cmd.name + "\n";
						std::this_thread::sleep_for(std::chrono::milliseconds(1000));
						bot.global_command_create(cmd);
					}
				
				event.reply(reply);
			}
		}
		if (message.substr(0, message.find(" ")) == "global_update_that" && author_id == owner_id) {
			if (dpp::run_once<struct cmd_reg>()) {
				std::vector<std::string> splited = split(message);
				auto cmds = build_commands(bot.me.id);
				std::string reply = "**commands updated:**\n";
				for (auto& cmd : cmds) {
					for (auto& splt : splited) {
						if (cmd.name == splt) {
							reply = reply + cmd.name + "\n";
							std::this_thread::sleep_for(std::chrono::milliseconds(1000));
							bot.global_command_create(cmd);
						}
					}
				}

				event.reply(reply);
			}
		}
		if (message.substr(0, message.find(" ")) == "guild_delete" && author_id == owner_id) {
			std::vector<std::string> splited = split(message);
			dpp::snowflake arg = std::stoull(splited[1]);
			if (fm.delete_guild(arg, "D:\\DEV\\Disbot\\tests\\")) {
				event.reply("Гильдия удалена!");
			}
			else {
				event.reply("Не получилось удалить гильдию!");
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
