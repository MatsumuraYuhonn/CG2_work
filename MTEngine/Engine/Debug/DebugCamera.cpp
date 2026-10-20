#include "DebugCamera.h"

#include <algorithm>
#include <cmath>

namespace MTEngine {

    namespace {

        constexpr float kPi = 3.14159265358979323846f;
        constexpr float kMaximumPitch = kPi * 0.5f - 0.01f;

    }

    void DebugCamera::Initialize() {
        Reset();
    }

    void DebugCamera::Reset() {
        target_ = { 0.0f, 0.0f, 2.134f };
        distance_ = 79.0f;
        yaw_ = 0.0f;
        pitch_ = 0.0f;
        matRot_ = MakeIdentityMatrix();
    }

    void DebugCamera::SetPosition(const Vector3& position) {
        const Vector3 currentPosition = GetPosition();
        target_ = {
            target_.x + position.x - currentPosition.x,
            target_.y + position.y - currentPosition.y,
            target_.z + position.z - currentPosition.z,
        };
    }

    void DebugCamera::SetTarget(const Vector3& target) {
        target_ = target;
    }

    void DebugCamera::SetDistance(float distance) {
        distance_ = (std::max)(distance, 0.1f);
    }

    void DebugCamera::SetYaw(float yaw) {
        yaw_ = yaw;
    }

    void DebugCamera::SetPitch(float pitch) {
        pitch_ = std::clamp(pitch, -kMaximumPitch, kMaximumPitch);
    }

    void DebugCamera::Update(
        const Input* input,
        bool enableControl,
        float mouseWheelDelta) {
        if (!input) {
            return;
        }

        POINT mousePosition{};
        GetCursorPos(&mousePosition);
        const Vector2 currentMousePosition = {
            static_cast<float>(mousePosition.x),
            static_cast<float>(mousePosition.y),
        };
        const Vector2 mouseDelta = {
            currentMousePosition.x - preMousePosition_.x,
            currentMousePosition.y - preMousePosition_.y,
        };
        preMousePosition_ = currentMousePosition;

        const bool isMiddleMouseDown = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
        const bool isShiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        const bool isCtrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;

        if (enableControl && isMiddleMouseDown && isShiftDown) {
            const float panSpeed = distance_ * 0.002f;
            Vector3 pan = { -mouseDelta.x * panSpeed, mouseDelta.y * panSpeed, 0.0f };
            target_ = Add(target_, TransformNormal(pan, matRot_));
        }
        else if (enableControl && isMiddleMouseDown && isCtrlDown) {
            distance_ += mouseDelta.y * distance_ * 0.01f;
        }
        else if (enableControl && isMiddleMouseDown) {
            constexpr float kOrbitSensitivity = 0.005f;
            yaw_ += mouseDelta.x * kOrbitSensitivity;
            pitch_ = std::clamp(
                pitch_ + mouseDelta.y * kOrbitSensitivity,
                -kMaximumPitch,
                kMaximumPitch);
        }

        if (enableControl && mouseWheelDelta != 0.0f) {
            distance_ -= mouseWheelDelta * distance_ * 0.1f;
        }
        if (distance_ < 0.1f) {
            distance_ = 0.1f;
        }

        if (enableControl && input->TriggerKey(DIK_NUMPAD1)) {
            yaw_ = 0.0f;
            pitch_ = 0.0f;
        }
        if (enableControl && input->TriggerKey(DIK_NUMPAD3)) {
            yaw_ = kPi * 0.5f;
            pitch_ = 0.0f;
        }
        if (enableControl && input->TriggerKey(DIK_NUMPAD7)) {
            yaw_ = 0.0f;
            pitch_ = kMaximumPitch;
        }

        // Yaw is always around world Y and pitch never reaches +/-90 degrees,
        // so the orbit axes cannot become parallel (gimbal lock).
        matRot_ = MakeRotationXMatrix(pitch_) * MakeRotationYMatrix(yaw_);
        const Matrix4x4 offsetMatrix = MakeTranslateMatrix({ 0.0f, 0.0f, -distance_ });
        const Matrix4x4 targetMatrix = MakeTranslateMatrix(target_);
        cameraMatrix = offsetMatrix * matRot_ * targetMatrix;
        viewMatrix = Inverse(cameraMatrix);
        projectionMatrix = MakePerspectiveFovMatrix(
            0.45f,
            static_cast<float>(kClientWidth) /
                static_cast<float>(kClientHeight),
            0.1f,
            1000.0f);
    }

}
