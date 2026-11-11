#pragma once

#include <cstdint>
#include <vector>

#include "MTEngine/Engine/Math/Vector.h"

namespace MTEngine {
    class Scene;
}

class PlayerBulletManager {
public:
    bool Initialize(MTEngine::Scene& scene);

    void Update(
        float deltaTime,
        const MTEngine::Vector3& playerPosition,
        const MTEngine::Vector3& aimDirection);

    void Clear();

private:
    struct Bullet {
        uint64_t objectId = 0;
        MTEngine::Vector3 direction{};
        float remainingLife = 0.0f;
    };

    void Fire(
        const MTEngine::Vector3& playerPosition,
        const MTEngine::Vector3& aimDirection);

    void UpdateBullets(float deltaTime);

    MTEngine::Scene* scene_ = nullptr;
    std::vector<Bullet> bullets_;

    float fireTimer_ = 0.0f;

    static constexpr float kFireInterval = 0.2f;
    static constexpr float kBulletSpeed = 15.0f;
    static constexpr float kBulletLifeTime = 3.0f;
    static constexpr float kSpawnDistance = 0.8f;
};