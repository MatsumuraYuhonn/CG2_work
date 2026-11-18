#include "GameCamera.h"

#include "MTEngine/Engine/Input/Input.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void GameCamera::Initialize(const MTEngine::Input& input) {
    MTEngine::DebugCamera::Initialize();
    SetDistance(kDistance);
    SetYaw(0.0f);
    SetPitch(kPitch);
    MTEngine::DebugCamera::Update(&input, false, 0.0f);
}

void GameCamera::Update(
    const MTEngine::Input& input,
    const MTEngine::Vector3& target) {

    SetTarget(target);
    MTEngine::DebugCamera::Update(&input, false, 0.0f);
}

void GameCamera::DrawInspector() {
#ifdef USE_IMGUI
    const MTEngine::Vector3 position = GetPosition();
    const MTEngine::Vector3 target = GetTarget();

    float distance = GetDistance();
    float yaw = GetYaw();
    float pitch = GetPitch();

    ImGui::Text("Game Camera");
    ImGui::Separator();

    ImGui::Text(
        "Position: %.2f, %.2f, %.2f",
        position.x,
        position.y,
        position.z);

    ImGui::Text(
        "Target: %.2f, %.2f, %.2f",
        target.x,
        target.y,
        target.z);

    ImGui::Separator();

    if (ImGui::DragFloat(
        "Distance",
        &distance,
        0.1f,
        0.1f,
        1000.0f)) {

        SetDistance(distance);
    }

    if (ImGui::SliderAngle(
        "Yaw",
        &yaw,
        -180.0f,
        180.0f)) {

        SetYaw(yaw);
    }

    if (ImGui::SliderAngle(
        "Pitch",
        &pitch,
        -89.0f,
        89.0f)) {

        SetPitch(pitch);
    }
#endif
}