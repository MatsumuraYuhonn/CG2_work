#include <chrono>
#include <memory>
#include <cmath>

#include "MTEngine/Engine/Base/Engine.h"
#include "MTEngine/Engine/Particle/ParticleEditor.h"
#include "MTEngine/Engine/Particle/ParticleEffectSerializer.h"
#include "MTEngine/Engine/Particle/ParticleSystem.h"
#include "MTEngine/Engine/Scene/SceneObject.h"
#include "MTEngine/Engine/Scene/SceneSerializer.h"
#include "MTEngine/Engine/Scene/Scene.h"

namespace {

    class GameApplication final : public MTEngine::Engine {
    protected:
        void OnInitialize() override {
            MTEngine::SceneSerializer::Load(
                kSceneFilePath,
                GetScene());

            for (const MTEngine::SceneObject& object : GetScene().GetObjects()) {
                if (object.name == "Player") {
                    playerId_ = object.id;
                    break;
                }
            }

            MTEngine::ParticleEffectData particleData;
            if (!MTEngine::ParticleEffectSerializer::Load(
                kParticleFilePath,
                particleData)) {
                MTEngine::ParticleEffectSerializer::Save(
                    kParticleFilePath,
                    particleData);
            }

            particleSystem_.Initialize(
                GetDevice(),
                GetRenderer()->GetSrvHeapManager(),
                GetCommandList(),
                particleData);

            particleEditor_.Initialize(
                &particleSystem_,
                kParticleFilePath,
                particleData);

            GetEditor()->RegisterGameObject(
                "Particle Emitter",
                [this]() {
                    particleEditor_.Draw();
                },
                [this]() {
                    particleEditor_.DrawSceneVisualization(
                        GetDebugCamera());
                });

            previousFrameTime_ = Clock::now();
        }

        void OnUpdate() override {
            const Clock::time_point currentTime = Clock::now();
            const std::chrono::duration<float> elapsed =
                currentTime - previousFrameTime_;
            previousFrameTime_ = currentTime;

            MTEngine::SceneObject* player =
                GetScene().FindObject(playerId_);

            if (player) {
                float moveX = 0.0f;
                float moveZ = 0.0f;

                MTEngine::Input* input = GetInput();

                // キーボード
                if (input->PushKey(DIK_W)) {
                    moveZ += 1.0f;
                }
                if (input->PushKey(DIK_S)) {
                    moveZ -= 1.0f;
                }
                if (input->PushKey(DIK_D)) {
                    moveX += 1.0f;
                }
                if (input->PushKey(DIK_A)) {
                    moveX -= 1.0f;
                }

                // コントローラー
                const MTEngine::GamePad* gamePad = input->GetGamePad();
                if (gamePad && gamePad->IsConnected()) {
                    const MTEngine::StickState& stick =
                        gamePad->GetLeftStick();

                    moveX += stick.x;
                    moveZ += stick.y;
                }

                // 斜め移動が速くならないよう正規化
                const float length =
                    std::sqrt(moveX * moveX + moveZ * moveZ);

                if (length > 1.0f) {
                    moveX /= length;
                    moveZ /= length;
                }

                player->transform.translate.x +=
                    moveX * kPlayerSpeed * elapsed.count();

                player->transform.translate.z +=
                    moveZ * kPlayerSpeed * elapsed.count();

                // 移動方向を向かせる場合
                if (length > 0.001f) {
                    player->transform.rotate.y =
                        std::atan2(moveX, moveZ);
                }

                // デバッグカメラで追従する場合
                GetDebugCamera().SetTarget(
                    player->transform.translate);
            }

            particleSystem_.Update(elapsed.count());
        }

        void OnDraw() override {
            particleSystem_.Draw(
                GetRenderer(),
                GetDebugCamera());
        }

        void OnFinalize() override {
            particleEditor_.SaveCurrentData();
            MTEngine::SceneSerializer::Save(
                kSceneFilePath,
                GetScene());
        }

    private:
        static constexpr const char* kSceneFilePath =
            "MTEngine/Assets/Scenes/main.scene.json";

        static constexpr const char* kParticleFilePath =
            "MTEngine/Assets/Particles/fire.json";

        using Clock = std::chrono::steady_clock;

        MTEngine::ParticleSystem particleSystem_;
        MTEngine::ParticleEditor particleEditor_;
        Clock::time_point previousFrameTime_{};

        uint64_t playerId_ = 0;
        static constexpr float kPlayerSpeed = 5.0f;

    };
}

int WINAPI WinMain(
    _In_ HINSTANCE,
    _In_opt_ HINSTANCE,
    _In_ LPSTR,
    _In_ int) {

    auto game = std::make_unique<GameApplication>();

    game->Initialize();
    game->Run();
    game->Finalize();

    return 0;
}
