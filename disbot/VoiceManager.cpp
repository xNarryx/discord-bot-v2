#include "VoiceManager.h"

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	std::ofstream* file = static_cast<std::ofstream*>(userp);
	file->write(static_cast<char*>(contents), size * nmemb);
	return size * nmemb;
}
std::string escape_json(const std::string& input) {
	std::string out;
	for (char c : input) {
		switch (c) {
		case '\"': out += "\\\""; break;
		case '\\': out += "\\\\"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default: out += c;
		}
	}
	return out;
}

std::vector<uint8_t> VoiceManager::to_pcmdata(std::string& way, float volume)
{
	std::vector<int16_t> pcm_samples; // сначала собираем в int16_t

	int err = 0;
	mpg123_handle* mh = mpg123_new(NULL, &err);
	if (!mh) return {};

	if (mpg123_open(mh, way.c_str()) != MPG123_OK) {
		mpg123_delete(mh);
		return {};
	}

	long rate;
	int channels, encoding;
	mpg123_getformat(mh, &rate, &channels, &encoding);

	size_t buffer_size = mpg123_outblock(mh);
	std::vector<unsigned char> buffer(buffer_size);
	size_t done = 0;

	while (mpg123_read(mh, buffer.data(), buffer_size, &done) == MPG123_OK) {
		int16_t* samples = reinterpret_cast<int16_t*>(buffer.data());
		size_t sample_count = done / sizeof(int16_t);

		for (size_t i = 0; i < sample_count; i++) {
			float scaled = samples[i] * volume;
			if (scaled > 32767.0f) scaled = 32767.0f;
			if (scaled < -32768.0f) scaled = -32768.0f;
			pcm_samples.push_back(static_cast<int16_t>(scaled));
		}
	}

	mpg123_close(mh);
	mpg123_delete(mh);

	// Если частота != 48000, делаем ресемплинг через libsamplerate
	if (rate != 48000) {
		double src_ratio = 48000.0 / rate;
		std::vector<int16_t> resampled((size_t)(pcm_samples.size() * src_ratio) + 1);

		SRC_DATA src_data;
		src_data.data_in = reinterpret_cast<const float*>(pcm_samples.data());
		src_data.input_frames = pcm_samples.size() / channels;
		src_data.data_out = reinterpret_cast<float*>(resampled.data());
		src_data.output_frames = resampled.size() / channels;
		src_data.src_ratio = src_ratio;
		src_data.end_of_input = 1;

		// конвертируем int16 -> float для SRC
		std::vector<float> float_in(pcm_samples.size());
		for (size_t i = 0; i < pcm_samples.size(); i++)
			float_in[i] = pcm_samples[i] / 32768.0f;

		src_data.data_in = float_in.data();
		int err = src_simple(&src_data, SRC_SINC_MEDIUM_QUALITY, channels);
		if (err != 0) {
			std::cerr << "Resampling error: " << src_strerror(err) << std::endl;
			return {};
		}

		// конвертируем обратно в int16
		for (size_t i = 0; i < src_data.output_frames_gen * channels; i++) {
			float sample = src_data.data_out[i];
			if (sample > 1.0f) sample = 1.0f;
			if (sample < -1.0f) sample = -1.0f;
			resampled[i] = static_cast<int16_t>(sample * 32767);
		}

		// возвращаем как uint8_t
		std::vector<uint8_t> pcmdata(resampled.size() * sizeof(int16_t));
		memcpy(pcmdata.data(), resampled.data(), pcmdata.size());
		return pcmdata;
	}

	// если уже 48000, просто возвращаем как uint8_t
	std::vector<uint8_t> pcmdata(pcm_samples.size() * sizeof(int16_t));
	memcpy(pcmdata.data(), pcm_samples.data(), pcmdata.size());
	return pcmdata;
}

bool VoiceManager::join_voice(dpp::snowflake id_user, dpp::event_dispatch_t event, dpp::snowflake guild_id, dpp::snowflake id_channel)
{
	dpp::guild* g = dpp::find_guild(guild_id);
		if (id_channel == 0) {
			if (g->connect_member_voice(*event.owner,id_user)) {
				const auto& vs = g->voice_members.find(id_user);
				voice_id_joined.insert_or_assign(guild_id ,vs->second.channel_id);
				return true;
			}
		}
		else {
			for (const auto& [id, state] : g->voice_members) {
				if (state.channel_id == id_channel) {
					g->connect_member_voice(*event.owner, id_user);
					voice_id_joined.insert_or_assign(guild_id, id_channel);
					return true;
				}
			}
		}
		
	return false;
}

bool VoiceManager::leave_voice(dpp::event_dispatch_t event, dpp::snowflake guild_id, dpp::snowflake channel_id)
{
	if (voice_id_joined.contains(guild_id)) {
		voice_id_joined.erase(guild_id);
		event.from()->disconnect_voice(guild_id);
		return true;
	}
	return false;
}

std::unordered_map<dpp::snowflake, dpp::snowflake> VoiceManager::check_voices()
{
	return std::unordered_map<dpp::snowflake, dpp::snowflake>(voice_id_joined);
}

bool VoiceManager::is_in_voice_here(dpp::snowflake guild)
{
	if (voice_id_joined.contains(guild)) {
		return true;
	}
	return false;
}

bool VoiceManager::play(std::string way, dpp::snowflake guild, dpp::event_dispatch_t event, float volume)
{
	dpp::voiceconn* v = event.from()->get_voice(guild);
	if (v && v->is_ready()) {
		std::vector<uint8_t> pcmdata = to_pcmdata(way);
		v->voiceclient->send_audio_raw((uint16_t*)pcmdata.data(), pcmdata.size());
		return true;
	}

	return false;
}

std::string VoiceManager::tts_create(std::string text, User* user, std::string file_name, std::string file_path)
{
	if (user->get_user_tts_voice().substr(0, 2) == user->get_user_tts_voice().substr(0, 2)) {
		std::string apiKey = "85ae882f70bc42feabb20b913ad11dd8";

		CURL* curl = curl_easy_init();
		if (!curl) {
			std::cerr << "Ошибка инициализации CURL!" << std::endl;
			return "";
		}
		// &hl=ru-ru&v=Peter
		char* escaped_text = curl_easy_escape(curl, text.c_str(), 0);

		if (!escaped_text) {
			std::cerr << "Ошибка кодирования текста!" << std::endl;
			curl_easy_cleanup(curl);
			return "";
		}

		std::string url = "http://api.voicerss.org/?key=" + apiKey + user->get_user_tts_voice() + "&c=MP3&r=1&f=48khz_16bit_stereo&src=" + escaped_text;
		curl_free(escaped_text);

		std::string path = file_path + file_name + ".MP3";
		std::ofstream file(path, std::ios::binary);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);

		CURLcode res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		file.close();

		return path;
	}
	else {
		std::string apiKey = "sk_c883d9bbf42165f41be1601423b8d4203a09d25c892b11fb";
		std::string voice_id = user->get_user_tts_voice(); // ID озвучки у пользователя
		
		voice_id = "Xb7hH8MSUJpSbSDYk0k2";

		std::string url = "https://api.elevenlabs.io/v1/text-to-speech/" + voice_id + "?output_format=mp3_44100_128";

		CURL* curl = curl_easy_init();
		if (!curl) return "";

		std::string json = R"({"text":")" + text + R"(","model_id":"eleven_multilingual_v2"})";
		std::string path = file_path + file_name + ".mp3";

		std::ofstream file(path, std::ios::binary);

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, nullptr);
		curl_slist* headers = nullptr;
		headers = curl_slist_append(headers, ("xi-api-key: " + apiKey).c_str());
		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);

		CURLcode res = curl_easy_perform(curl);
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
		file.close();

		if (res != CURLE_OK) {
			std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
			return "";
		}

		// проверяем что файл существует и не пустой
		std::ifstream f(path, std::ios::binary | std::ios::ate);
		if (!f || f.tellg() == 0) {
			std::cerr << "TTS returned empty file!" << std::endl;
			return "";
		}

		return path;

		
	}
}
