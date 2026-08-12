#pragma once
#include <Engine/Engine.hpp>

class StudentPlayerEntity; // Forward declaration

enum class TeacherBuffType : uint8_t {
    SpeedBoost,
    DeadlineShield,
    GpaMultiplier
};

class TeacherNPCEntity : public Entity2D {
private:
    StudentPlayerEntity* m_player = nullptr; // <--- Add player pointer
    TeacherBuffType m_buffType = TeacherBuffType::SpeedBoost;
    glm::vec2 m_patrolA{ 0.0f, 0.0f };
    glm::vec2 m_patrolB{ 0.0f, 0.0f };
    bool m_movingToB = true;
    bool m_isBuffGiven = false;
    AudioHandle m_buffSFX{};

    float m_moveSpeed = 80.0f;
    glm::vec4 m_color{ 0.1f, 0.7f, 0.9f, 1.0f };
    int32_t m_renderDepth = 35;

public:
    TeacherNPCEntity(const glm::vec2& pos,
        const glm::vec2& patrolB,
        TeacherBuffType buff = TeacherBuffType::SpeedBoost);

    void setPlayer(StudentPlayerEntity* player) noexcept { m_player = player; } // <--- Setter

    void onAttach(EngineContext* ctx);
    void onUpdate(float dt, EngineContext* ctx) override;
    void onCollision(const CollisionInfo& collision, EngineContext* ctx) override;
    void onTrigger(const CollisionInfo& trigger, EngineContext* ctx) override;
    void onRender(RenderData& writeBuffer, EngineContext* ctx, const glm::vec2& renderPos) override;

    [[nodiscard]] TeacherBuffType getBuffType() const noexcept { return m_buffType; }
    [[nodiscard]] bool isBuffGiven() const noexcept { return m_isBuffGiven; }
};