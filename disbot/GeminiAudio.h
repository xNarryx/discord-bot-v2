#pragma once

#include <string>
#include "AudioData.h"

class GeminiAudio
{
public:
    static AudioData Parse(const std::string& json_response);
};