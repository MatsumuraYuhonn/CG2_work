#pragma once

#include <cstdint>

namespace MTEngine {
    class Input;
    class Scene;
    struct SceneObject;
}

class Player {
public:
    bool Initialize(
        MTEngine::Scene& scene,
        MTEngine::Input& input);

    void Update(float deltaTime);

    const MTEngine::SceneObject* GetSceneObject() const;

private:
    MTEngine::Scene* scene_ = nullptr;
    MTEngine::Input* input_ = nullptr;
    uint64_t objectId_ = 0;

    static constexpr float kMoveSpeed = 5.0f;
};

