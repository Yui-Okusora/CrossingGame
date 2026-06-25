#pragma once
#include <glm/glm.hpp>

// src/Engine/UI/UIAnchor.hpp
#pragma once
#include <glm/glm.hpp>
#include <cstdint>

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