#pragma once
#include <Windows.h>
#include "MTEngine/Engine/Math/Vector.h"
#include "MTEngine/Engine/Math/Matrix.h"
#include "MTEngine/Engine/Base/Window.h"
#include "MTEngine/Engine/Input/Input.h"

// デバッグ用のカメラ操作クラス。マウスやキーボード入力による視点操作を提供する
namespace MTEngine {

    class DebugCamera {
    public:
        // カメラの初期化
        void Initialize();

        // ターゲット、距離、向きを初期状態へ戻す
        void Reset();

        // 入力に基づいたカメラの更新処理
        void Update(
            const Input* input,
            bool enableControl = true,
            float mouseWheelDelta = 0.0f);

        // ビュー行列の取得
        const Matrix4x4& GetViewMatrix() const { return viewMatrix; }

        // プロジェクション行列の取得
        const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix; }

        Vector3 GetPosition() const {
            return {
                cameraMatrix.m[3][0],
                cameraMatrix.m[3][1],
                cameraMatrix.m[3][2],
            };
        }
        const Vector3& GetTarget() const { return target_; }
        float GetDistance() const { return distance_; }
        float GetYaw() const { return yaw_; }
        float GetPitch() const { return pitch_; }

        void SetPosition(const Vector3& position);
        void SetTarget(const Vector3& target);
        void SetDistance(float distance);
        void SetYaw(float yaw);
        void SetPitch(float pitch);

    private:
        Matrix4x4 cameraMatrix;
        Matrix4x4 viewMatrix{};
        Matrix4x4 projectionMatrix;

        Matrix4x4 matRot_;

        Vector3 target_ = { 0.0f, 0.0f, 0.0f };
        float distance_ = 50.0f;
        float yaw_ = 0.0f;
        float pitch_ = 0.0f;

        Vector2 preMousePosition_ = { 0.0f, 0.0f };
    };

}
