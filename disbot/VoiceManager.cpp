#include "VoiceManager.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

static std::string ffmpeg_error(int error)
{
	char buffer[AV_ERROR_MAX_STRING_SIZE];

	av_strerror(
		error,
		buffer,
		sizeof(buffer)
	);

	return std::string(buffer);
}


size_t WriteCallbackFile(void* contents, size_t size, size_t nmemb, void* userp) {
	std::ofstream* file = static_cast<std::ofstream*>(userp);
	file->write(static_cast<char*>(contents), size * nmemb);
	return size * nmemb;
}
size_t WriteCallbackSTR(void* contents, size_t size, size_t nmemb, void* userp)
{
	std::string* response = static_cast<std::string*>(userp);
	response->append(static_cast<char*>(contents), size * nmemb);
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


dpp::snowflake VoiceManager::get_voice_channel(dpp::snowflake guild)
{
	if (voice_id_joined.contains(guild)) {
		return voice_id_joined[guild];
	}
	else {
		return 0;
	}
}

void VoiceManager::add_api_keys(std::unordered_map <std::string, std::string> api_keys){
	this->api_keys = api_keys;
}

std::vector<uint8_t> VoiceManager::to_pcmdata(
	const std::string& path,
	float volume
)
{
	constexpr int OUTPUT_SAMPLE_RATE = 48000;
	constexpr int OUTPUT_CHANNELS = 2;

	std::vector<int16_t> pcm_samples;

	AVFormatContext* format_ctx = nullptr;

	int ret = avformat_open_input(
		&format_ctx,
		path.c_str(),
		nullptr,
		nullptr
	);

	if (ret < 0)
	{
		char error[AV_ERROR_MAX_STRING_SIZE];

		av_strerror(
			ret,
			error,
			sizeof(error)
		);

		std::cerr
			<< "Failed to open audio file: "
			<< path
			<< "\nFFmpeg error: "
			<< error
			<< '\n';

		return {};
	}


	ret = avformat_find_stream_info(
		format_ctx,
		nullptr
	);

	if (ret < 0)
	{
		std::cerr
			<< "Failed to find stream information\n";

		avformat_close_input(&format_ctx);

		return {};
	}


	int audio_stream_index = av_find_best_stream(
		format_ctx,
		AVMEDIA_TYPE_AUDIO,
		-1,
		-1,
		nullptr,
		0
	);

	if (audio_stream_index < 0)
	{
		std::cerr
			<< "No audio stream found in file: "
			<< path
			<< '\n';

		avformat_close_input(&format_ctx);

		return {};
	}

	AVStream* audio_stream =
		format_ctx->streams[audio_stream_index];


	const AVCodec* codec =
		avcodec_find_decoder(
			audio_stream->codecpar->codec_id
		);

	if (!codec)
	{
		std::cerr
			<< "Could not find decoder for audio stream\n";

		avformat_close_input(&format_ctx);

		return {};
	}


	AVCodecContext* codec_ctx =
		avcodec_alloc_context3(codec);

	if (!codec_ctx)
	{
		std::cerr
			<< "Could not allocate codec context\n";

		avformat_close_input(&format_ctx);

		return {};
	}

	ret = avcodec_parameters_to_context(
		codec_ctx,
		audio_stream->codecpar
	);

	if (ret < 0)
	{
		std::cerr
			<< "Could not copy codec parameters\n";

		avcodec_free_context(&codec_ctx);
		avformat_close_input(&format_ctx);

		return {};
	}


	ret = avcodec_open2(
		codec_ctx,
		codec,
		nullptr
	);

	if (ret < 0)
	{
		std::cerr
			<< "Could not open audio decoder\n";

		avcodec_free_context(&codec_ctx);
		avformat_close_input(&format_ctx);

		return {};
	}


	AVChannelLayout output_layout =
		AV_CHANNEL_LAYOUT_STEREO;

	SwrContext* swr_ctx = nullptr;

	ret = swr_alloc_set_opts2(
		&swr_ctx,

		// OUTPUT
		&output_layout,
		AV_SAMPLE_FMT_S16,
		OUTPUT_SAMPLE_RATE,

		// INPUT
		&codec_ctx->ch_layout,
		codec_ctx->sample_fmt,
		codec_ctx->sample_rate,

		0,
		nullptr
	);

	if (ret < 0 || !swr_ctx)
	{
		std::cerr
			<< "Could not create resampler\n";

		swr_free(&swr_ctx);
		avcodec_free_context(&codec_ctx);
		avformat_close_input(&format_ctx);

		return {};
	}

	ret = swr_init(swr_ctx);

	if (ret < 0)
	{
		std::cerr
			<< "Could not initialize resampler\n";

		swr_free(&swr_ctx);
		avcodec_free_context(&codec_ctx);
		avformat_close_input(&format_ctx);

		return {};
	}


	AVPacket* packet = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();

	if (!packet || !frame)
	{
		std::cerr
			<< "Could not allocate FFmpeg packet/frame\n";

		av_packet_free(&packet);
		av_frame_free(&frame);

		swr_free(&swr_ctx);
		avcodec_free_context(&codec_ctx);
		avformat_close_input(&format_ctx);

		return {};
	}


	while ((ret = av_read_frame(format_ctx, packet)) >= 0)
	{
		if (packet->stream_index != audio_stream_index)
		{
			av_packet_unref(packet);
			continue;
		}

		ret = avcodec_send_packet(
			codec_ctx,
			packet
		);

		av_packet_unref(packet);

		if (ret < 0)
		{
			std::cerr
				<< "Error sending packet to decoder\n";

			continue;
		}

		while (true)
		{
			ret = avcodec_receive_frame(
				codec_ctx,
				frame
			);

			if (ret == AVERROR(EAGAIN) ||
				ret == AVERROR_EOF)
			{
				break;
			}

			if (ret < 0)
			{
				std::cerr
					<< "Error receiving decoded frame\n";

				break;
			}


			int output_samples =
				av_rescale_rnd(
					swr_get_delay(
						swr_ctx,
						codec_ctx->sample_rate
					) + frame->nb_samples,

					OUTPUT_SAMPLE_RATE,
					codec_ctx->sample_rate,

					AV_ROUND_UP
				);

			if (output_samples <= 0)
				continue;

			std::vector<int16_t> converted(
				output_samples * OUTPUT_CHANNELS
			);

			uint8_t* output_data =
				reinterpret_cast<uint8_t*>(
					converted.data()
					);


			int converted_samples =
				swr_convert(
					swr_ctx,

					&output_data,
					output_samples,

					const_cast<const uint8_t**>(
						frame->extended_data
						),
					frame->nb_samples
				);

			if (converted_samples < 0)
			{
				std::cerr
					<< "Error while resampling audio\n";

				continue;
			}

			converted.resize(
				converted_samples * OUTPUT_CHANNELS
			);

			pcm_samples.insert(
				pcm_samples.end(),
				converted.begin(),
				converted.end()
			);
		}
	}


	ret = avcodec_send_packet(
		codec_ctx,
		nullptr
	);

	if (ret >= 0)
	{
		while (true)
		{
			ret = avcodec_receive_frame(
				codec_ctx,
				frame
			);

			if (ret == AVERROR_EOF ||
				ret == AVERROR(EAGAIN))
			{
				break;
			}

			if (ret < 0)
				break;

			int output_samples =
				av_rescale_rnd(
					swr_get_delay(
						swr_ctx,
						codec_ctx->sample_rate
					) + frame->nb_samples,

					OUTPUT_SAMPLE_RATE,
					codec_ctx->sample_rate,

					AV_ROUND_UP
				);

			if (output_samples <= 0)
				continue;

			std::vector<int16_t> converted(
				output_samples * OUTPUT_CHANNELS
			);

			uint8_t* output_data =
				reinterpret_cast<uint8_t*>(
					converted.data()
					);

			int converted_samples =
				swr_convert(
					swr_ctx,
					&output_data,
					output_samples,

					const_cast<const uint8_t**>(
						frame->extended_data
						),
					frame->nb_samples
				);

			if (converted_samples < 0)
				continue;

			converted.resize(
				converted_samples * OUTPUT_CHANNELS
			);

			pcm_samples.insert(
				pcm_samples.end(),
				converted.begin(),
				converted.end()
			);
		}
	}


	int delayed_samples =
		av_rescale_rnd(
			swr_get_delay(
				swr_ctx,
				codec_ctx->sample_rate
			),

			OUTPUT_SAMPLE_RATE,
			codec_ctx->sample_rate,

			AV_ROUND_UP
		);

	if (delayed_samples > 0)
	{
		std::vector<int16_t> converted(
			delayed_samples * OUTPUT_CHANNELS
		);

		uint8_t* output_data =
			reinterpret_cast<uint8_t*>(
				converted.data()
				);

		int converted_samples =
			swr_convert(
				swr_ctx,
				&output_data,
				delayed_samples,

				nullptr,
				0
			);

		if (converted_samples > 0)
		{
			converted.resize(
				converted_samples * OUTPUT_CHANNELS
			);

			pcm_samples.insert(
				pcm_samples.end(),
				converted.begin(),
				converted.end()
			);
		}
	}

	av_packet_free(&packet);
	av_frame_free(&frame);

	swr_free(&swr_ctx);

	avcodec_free_context(&codec_ctx);

	avformat_close_input(&format_ctx);


	if (pcm_samples.empty())
	{
		std::cerr
			<< "No PCM samples decoded from: "
			<< path
			<< '\n';

		return {};
	}


	for (int16_t& sample : pcm_samples)
	{
		float value =
			static_cast<float>(sample) * volume;

		value = std::clamp(
			value,
			-32768.0f,
			32767.0f
		);

		sample =
			static_cast<int16_t>(value);
	}


	std::vector<uint8_t> pcmdata(
		pcm_samples.size() * sizeof(int16_t)
	);

	std::memcpy(
		pcmdata.data(),
		pcm_samples.data(),
		pcmdata.size()
	);

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
			std::vector<uint8_t> pcmdata = to_pcmdata(way, volume);
			std::cout << "PCM size: "
				<< pcmdata.size()
				<< " bytes\n";
			v->voiceclient->send_audio_raw((uint16_t*)pcmdata.data(), pcmdata.size());
			return true;
	}

	return false;
}

std::string VoiceManager::tts_create(std::string text, User* user, std::string file_name, std::string file_path, std::string prompt)
{
	std::string check1 = "&hl=ru";
	if (user->get_user_tts_voice().substr(0, 2) == check1.substr(0, 2)) {
		std::string apiKey = get_api_keyy("rss");

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
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackFile);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);

		CURLcode res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		file.close();

		return path;
	}
	else 
		if (user->get_user_tts_voice().substr(0, user->get_user_tts_voice().find(" ")) == "Gemini") {
			std::string Voice = user->get_user_tts_voice().substr(user->get_user_tts_voice().find(" ")+1);
			std::vector<std::string> models = 
			{ "gemini-3.1-flash-tts-preview", 
			"gemini-2.5-flash-preview-tts" };
			for (auto& model : models) {
				CURL* curl = curl_easy_init();
				nlohmann::json body = {
					{"model", "gemini-3.1-flash-tts-preview"},
					{"input", prompt + ": " + text},
					{"response_format", {
						{"type", "audio"}
					}},
					{"generation_config", {
						{"speech_config", {
							{ {"voice", Voice} }
						}}
						}}
				};
				std::cout << "Created request for gemini TTS\n";
				std::string json_str = body.dump();
				struct curl_slist* headers = nullptr;
				headers = curl_slist_append(headers, ("x-goog-api-key: " + get_api_keyy("gemini")).c_str());
				headers = curl_slist_append(headers, "Content-Type: application/json");
				curl_easy_setopt(curl, CURLOPT_URL,
					"https://generativelanguage.googleapis.com/v1beta/interactions");
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackSTR);
				std::string path = file_path + file_name + ".wav";
				std::string str;
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str);

				CURLcode res = curl_easy_perform(curl);
				if (res == CURLE_OK)
				{
					AudioData audio = GeminiAudio::Parse(str);

					if (audio.usable) {

						WavWriter::Save(audio, path);
					}
					else {
						continue;
					}
				}
				else
				{
					std::cout << "Curl error: " << curl_easy_strerror(res) << std::endl;
					return "";
				}
				curl_slist_free_all(headers);
				curl_easy_cleanup(curl);

				return path;
			}
		}
		else {
			return "";
		}
	
}

