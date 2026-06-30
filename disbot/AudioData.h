#pragma once
#include <vector>
#include <cstdint>


struct AudioData
{
    std::vector<int16_t> samples;

    uint32_t sample_rate = 24000;
    uint16_t channels = 1;
    uint16_t bits_per_sample = 16;
    bool usable = true;
};