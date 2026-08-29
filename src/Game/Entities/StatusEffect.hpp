#pragma once
#include <Engine/Engine.hpp>
#include <memory>
#include <vector>
#include <algorithm>

class StudentPlayerEntity;

enum class TeacherBuffType : uint8_t {
    SpeedBoost,
    DeadlineShield,
    GpaMultiplier,
    Invincibility
};

class IStatusEffect {
public:
    virtual ~IStatusEffect() = default;
    virtual void onApply(StudentPlayerEntity* player) {}
    virtual void onUpdate(float dt, StudentPlayerEntity* player) = 0;
    virtual void onRemove(StudentPlayerEntity* player) {}
    virtual bool onTakeDamage(StudentPlayerEntity* player) { return false; }
    [[nodiscard]] virtual bool isExpired() const noexcept = 0;
    [[nodiscard]] virtual TeacherBuffType getType() const noexcept = 0;
    [[nodiscard]] virtual float getRemainingTime() const noexcept { return 0.0f; }
};

// --- CONCRETE STATUS EFFECTS ---
class SpeedBoostEffect : public IStatusEffect {
private:
    float m_duration = 10.0f;
public:
    void onApply(StudentPlayerEntity* player) override;
    void onUpdate(float dt, StudentPlayerEntity* player) override { m_duration -= dt; }
    void onRemove(StudentPlayerEntity* player) override;
    [[nodiscard]] bool isExpired() const noexcept override { return m_duration <= 0.0f; }
    [[nodiscard]] TeacherBuffType getType() const noexcept override { return TeacherBuffType::SpeedBoost; }
    [[nodiscard]] float getRemainingTime() const noexcept override { return (std::max)(0.0f, m_duration); }
};

class GpaMultiplierEffect : public IStatusEffect {
private:
    float m_duration = 12.0f;
public:
    void onApply(StudentPlayerEntity* player) override;
    void onUpdate(float dt, StudentPlayerEntity* player) override { m_duration -= dt; }
    void onRemove(StudentPlayerEntity* player) override;
    [[nodiscard]] bool isExpired() const noexcept override { return m_duration <= 0.0f; }
    [[nodiscard]] TeacherBuffType getType() const noexcept override { return TeacherBuffType::GpaMultiplier; }
    [[nodiscard]] float getRemainingTime() const noexcept override { return (std::max)(0.0f, m_duration); }
};

class InvincibilityEffect : public IStatusEffect {
private:
    float m_duration = 2.0f;
public:
    explicit InvincibilityEffect(float duration = 2.0f) : m_duration(duration) {}
    void onUpdate(float dt, StudentPlayerEntity* player) override { m_duration -= dt; }
    bool onTakeDamage(StudentPlayerEntity* player) override { return true; } // Completely blocks damage
    [[nodiscard]] bool isExpired() const noexcept override { return m_duration <= 0.0f; }
    [[nodiscard]] TeacherBuffType getType() const noexcept override { return TeacherBuffType::Invincibility; }
    [[nodiscard]] float getRemainingTime() const noexcept override { return (std::max)(0.0f, m_duration); }
};

class DeadlineShieldEffect : public IStatusEffect {
private:
    bool m_consumed = false;
public:
    void onUpdate(float dt, StudentPlayerEntity* player) override {}
    bool onTakeDamage(StudentPlayerEntity* player) override {
        if (!m_consumed) {
            m_consumed = true; // Shield absorbs the lethal hit
            return true;
        }
        return false;
    }
    void onRemove(StudentPlayerEntity* player) override; // Defined in StudentPlayerEntity.cpp to grant invincibility
    [[nodiscard]] bool isExpired() const noexcept override { return m_consumed; }
    [[nodiscard]] TeacherBuffType getType() const noexcept override { return TeacherBuffType::DeadlineShield; }
};

// --- COMPOSITE CONTAINER (COEXISTING BUFFS) ---
class StatusEffectComposite : public IStatusEffect {
private:
    std::vector<std::unique_ptr<IStatusEffect>> m_effects;

public:
    void addEffect(std::unique_ptr<IStatusEffect> effect, StudentPlayerEntity* player) {
        // Refresh only the effect of the exact same type; all different buffs coexist together
        auto it = std::find_if(m_effects.begin(), m_effects.end(),
            [&](const auto& e) { return e->getType() == effect->getType(); });

        if (it != m_effects.end()) {
            (*it)->onRemove(player);
            m_effects.erase(it);
        }

        effect->onApply(player);
        m_effects.push_back(std::move(effect));
    }

    void onUpdate(float dt, StudentPlayerEntity* player) override {
        for (auto& effect : m_effects) {
            effect->onUpdate(dt, player);
        }
        cleanupExpired(player);
    }

    bool onTakeDamage(StudentPlayerEntity* player) override {
        for (auto& effect : m_effects) {
            if (effect->onTakeDamage(player)) {
                cleanupExpired(player);
                return true; // Damage successfully mitigated
            }
        }
        return false;
    }

    void cleanupExpired(StudentPlayerEntity* player) {
        for (auto& effect : m_effects) {
            if (effect->isExpired()) {
                effect->onRemove(player);
            }
        }
        std::erase_if(m_effects, [](const auto& e) { return e->isExpired(); });
    }

    void syncBlackboard(EngineContext* ctx) const {
        if (!ctx) return;
        ctx->blackboard.set("buffShield", hasEffect(TeacherBuffType::DeadlineShield));
        ctx->blackboard.set("buffSpeedTimer", getRemainingTime(TeacherBuffType::SpeedBoost));
        ctx->blackboard.set("buffGpaTimer", getRemainingTime(TeacherBuffType::GpaMultiplier));
        ctx->blackboard.set("buffInvincibleTimer", getRemainingTime(TeacherBuffType::Invincibility));
    }

    [[nodiscard]] bool isExpired() const noexcept override { return false; }
    [[nodiscard]] TeacherBuffType getType() const noexcept override { return TeacherBuffType::SpeedBoost; }

    [[nodiscard]] bool hasEffect(TeacherBuffType type) const noexcept {
        return std::any_of(m_effects.begin(), m_effects.end(),
            [type](const auto& e) { return e->getType() == type; });
    }

    [[nodiscard]] float getRemainingTime(TeacherBuffType type) const noexcept {
        for (const auto& e : m_effects) {
            if (e->getType() == type) return e->getRemainingTime();
        }
        return 0.0f;
    }

    void clear(StudentPlayerEntity* player) {
        for (auto& e : m_effects) e->onRemove(player);
        m_effects.clear();
    }
};