#pragma once

#include "MTEngine/Engine/Debug/DebugCamera.h"

namespace MTEngine {
    class Input;
}

class GameCamera : public MTEngine::DebugCamera {
public:
    void Initialize(const MTEngine::Input& input);

    void Update(
        const MTEngine::Input& input,
        const MTEngine::Vector3& target);

    void DrawInspector();

private:
    static constexpr float kDistance = 50.0f;
    static constexpr float kPitch = 1.0f;
};
