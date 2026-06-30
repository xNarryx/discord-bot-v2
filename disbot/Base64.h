#pragma once
#include <string>
#include <vector>

class Base64
{
public:
    static std::vector<uint8_t> Decode(const std::string& input);
};