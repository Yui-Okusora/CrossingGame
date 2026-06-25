#pragma once

#include <Core.hpp>
#include "CRC32_64/CRC32_64.hpp"
#include <chrono>

class Utils
{
public:
    static bool inRect(glm::vec2 pos, glm::vec2 rectPos, glm::vec2 rectDim)
    {
        return glm::all(glm::greaterThanEqual(pos, rectPos)) && glm::all(glm::lessThanEqual(pos, rectPos + rectDim));
    }
    static bool inRect(glm::vec2 pos, glm::vec2 objDim, glm::vec2 rectPos, glm::vec2 rectDim)
    {
        return glm::all(glm::greaterThanEqual(pos, rectPos)) && glm::all(glm::lessThanEqual(pos, rectPos + rectDim - objDim));
    }
    static glm::vec2 lerp2(glm::vec2 a, glm::vec2 b, float c, float distSnapThreshold = -1.0f)
    {
        a.x = std::lerp(a.x, b.x, c);
        a.y = std::lerp(a.y, b.y, c);

        if (std::abs(a.x - b.x) <= distSnapThreshold) a.x = b.x;
        if (std::abs(a.y - b.y) <= distSnapThreshold) a.y = b.y;

        return a;
    }

    static std::string timeToDate(double timestamp)
    {
        std::string time = "";
        time += std::to_string(long long(timestamp) / 3600) + ':';
        timestamp = double(long long(timestamp) % 3600) + (timestamp - (double)long long(timestamp));
        time += std::to_string(long long(timestamp) / 60) + ':';
        timestamp = double(long long(timestamp) % 60) + (timestamp - (double)long long(timestamp));
        time += std::to_string(long long(timestamp));

        return time;
    }

    static std::string formatTS(uint64_t timestampMs)
    {
        using namespace std::chrono;

        system_clock::time_point tp{ milliseconds(timestampMs) };

        std::time_t tt = system_clock::to_time_t(tp);

        std::tm tm{};

#ifdef _WIN32
        localtime_s(&tm, &tt);   // Windows (thread-safe)
#else
        localtime_r(&tt, &tm);   // POSIX (thread-safe)
#endif

        // Format
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

        return oss.str();
    }
};