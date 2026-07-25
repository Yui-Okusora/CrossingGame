#pragma once

#include <Core.hpp>
#include "CRC32_64/CRC32_64.hpp"
#include <chrono>

struct ViewportScale {
    float scale = 1.0f;
    glm::vec2 offset{ 0.0f, 0.0f };
};

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

    static inline ViewportScale ComputeViewportScale(glm::vec2 windowSize, glm::vec2 designSize) {
        float sx = windowSize.x / designSize.x;
        float sy = windowSize.y / designSize.y;

        float scale = (std::min)(sx, sy);
        glm::vec2 scaled = designSize * scale;
        glm::vec2 offset = (windowSize - scaled) * 0.5f;

        return { scale, offset };
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

enum class UIAnchor : uint8_t {
    TopLeft, TopCenter, TopRight,
    LeftCenter, Center, RightCenter,
    BottomLeft, BottomCenter, BottomRight
};

class UIAnchorEngine {
public:
    static inline glm::vec4 CalculateBounds(glm::vec2 canvasSize, glm::vec2 size, glm::vec2 offset, UIAnchor anchor) noexcept {
        switch (anchor) {
        case UIAnchor::TopLeft:
            return { offset.x, offset.y, size.x, size.y };

        case UIAnchor::TopCenter:
            return { (canvasSize.x * 0.5f) - (size.x * 0.5f) + offset.x, offset.y, size.x, size.y };

        case UIAnchor::TopRight:
            return { canvasSize.x - size.x - offset.x, offset.y, size.x, size.y };

        case UIAnchor::LeftCenter:
            return { offset.x, (canvasSize.y * 0.5f) - (size.y * 0.5f) + offset.y, size.x, size.y };

        case UIAnchor::Center:
            return { (canvasSize.x * 0.5f) - (size.x * 0.5f) + offset.x,
                     (canvasSize.y * 0.5f) - (size.y * 0.5f) + offset.y, size.x, size.y };

        case UIAnchor::RightCenter:
            return { canvasSize.x - size.x - offset.x, (canvasSize.y * 0.5f) - (size.y * 0.5f) + offset.y, size.x, size.y };

        case UIAnchor::BottomLeft:
            return { offset.x, canvasSize.y - size.y - offset.y, size.x, size.y };

        case UIAnchor::BottomCenter:
            // FIXED: Now computes horizontal symmetry paired with bottom-edge vertical margins
            return { (canvasSize.x * 0.5f) - (size.x * 0.5f) + offset.x, canvasSize.y - size.y - offset.y, size.x, size.y };

        case UIAnchor::BottomRight:
            return { canvasSize.x - size.x - offset.x, canvasSize.y - size.y - offset.y, size.x, size.y };
        }
        return { offset.x, offset.y, size.x, size.y };
    }
};