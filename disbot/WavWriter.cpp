#include "WavWriter.h"

#include <fstream>

bool WavWriter::Save(const AudioData& audio,
    const std::string& path)
{
    std::ofstream file(path, std::ios::binary);

    if (!file.is_open())
        return false;

    uint32_t dataSize =
        static_cast<uint32_t>(audio.samples.size() * sizeof(int16_t));

    uint32_t fileSize = 36 + dataSize;

    uint32_t byteRate =
        audio.sample_rate *
        audio.channels *
        audio.bits_per_sample / 8;

    uint16_t blockAlign =
        audio.channels *
        audio.bits_per_sample / 8;

    file.write("RIFF", 4);
    file.write(reinterpret_cast<char*>(&fileSize), 4);
    file.write("WAVE", 4);

    uint16_t channels = audio.channels;
    uint32_t sample_rate = audio.sample_rate;
    uint16_t bits = audio.bits_per_sample;

    file.write("fmt ", 4);

    uint32_t subChunk1Size = 16;
    uint16_t audioFormat = 1;

    file.write(reinterpret_cast<const char*>(&subChunk1Size), 4);
    file.write(reinterpret_cast<const char*>(&audioFormat), 2);
    file.write(reinterpret_cast<const char*>(&channels), 2);
    file.write(reinterpret_cast<const char*>(&sample_rate), 4);
    file.write(reinterpret_cast<const char*>(&byteRate), 4);
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    file.write(reinterpret_cast<const char*>(&bits), 2);

    file.write("data", 4);
    file.write(reinterpret_cast<char*>(&dataSize), 4);

    file.write(
        reinterpret_cast<const char*>(audio.samples.data()),
        dataSize
    );

    file.close();

    return true;
}

