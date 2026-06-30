#include "GeminiAudio.h"
#include "Base64.h"
#include <nlohmann/json.hpp>

AudioData GeminiAudio::Parse(const std::string& json_response)
{
    nlohmann::json j = nlohmann::json::parse(json_response);

    

    AudioData audio;
    if (j["status"] = true) {
        std::string base64 =
            j["steps"][0]["content"][0]["data"].get<std::string>();

        std::vector<uint8_t> raw = Base64::Decode(base64);
        audio.sample_rate = 24000;
        audio.channels = 1;
        audio.bits_per_sample = 16;
        audio.usable = true;
        // PCM L16 -> int16_t
        audio.samples.resize(raw.size() / 2);

        memcpy(audio.samples.data(), raw.data(), raw.size());
    }
    else {
        audio.usable = false;
    }

    

    return audio;
}