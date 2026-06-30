#pragma once

#include <string>
#include "AudioData.h"

class WavWriter
{
public:
    static bool Save(const AudioData& audio,
        const std::string& path);
};