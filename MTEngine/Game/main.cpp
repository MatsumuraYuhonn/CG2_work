#include <chrono>
#include <memory>
#include <vector>

#include "MTEngine/Engine/Base/Engine.h"
#include "MTEngine/Engine/Particle/ParticleEditor.h"
#include "MTEngine/Engine/Particle/ParticleEffectSerializer.h"
#include "MTEngine/Engine/Particle/ParticleSystem.h"
#include "MTEngine/Engine/Scene/SceneObject.h"
#include "MTEngine/Engine/Scene/SceneSerializer.h"
#include "MTEngine/Engine/Scene/Scene.h"
#include "GameCamera.h"
#include "Player.h"

namespace {

    class GameApplication final : public MTEngine::Engine {
    protected:
        void OnInitialize() override {
            MTEngine::SceneSerializer::Load(
                kSceneFilePath,
                GetScene());

            player_.Initialize(
                GetScene(),
                *GetInput());

            gameCamera_.Initialize(*GetInput());
            SetActiveCamera(gameCamera_);

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

            const std::vector<MTEngine::ParticleSystem::EmitterId>
                emitterIds = particleSystem_.GetEmitterIds();
            if (!emitterIds.empty()) {
                playerEmitterId_ = emitterIds.front();
            }

            particleEditor_.Initialize(
                &particleSystem_,
                kParticleFilePath,
                particleData);

            GetEditor()->RegisterGameObject(
                "Game Camera",
                [this]() {
                    gameCamera_.DrawInspector();
                });

            GetEditor()->RegisterGameObject(
                "Particle Emitter",
                [this]() {
                    particleEditor_.Draw();
                },
                [this]() {
                    particleEditor_.DrawSceneVisualization(
                        GetActiveCamera());
                });

            previousFrameTime_ = Clock::now();
        }

        void OnUpdate() override {
            const Clock::time_point currentTime = Clock::now();
            const std::chrono::duration<float> elapsed =
                currentTime - previousFrameTime_;
            previousFrameTime_ = currentTime;

            const float deltaTime = elapsed.count();

            if (GetInput()->TriggerKey(DIK_F11)) {
                if (isGameCameraActive_) {
                    SetActiveCamera(GetDebugCamera());
                    isGameCameraActive_ = false;
                } else {
                    SetActiveCamera(gameCamera_);
                    isGameCameraActive_ = true;
                }
            }

            player_.Update(deltaTime);

            const MTEngine::SceneObject* playerObject =
                player_.GetSceneObject();

            if (playerObject) {
                gameCamera_.Update(
                    *GetInput(),
                    playerObject->transform.translate);

                if (playerEmitterId_ != 0) {
                    particleSystem_.SetEmitterPosition(
                        playerEmitterId_,
                        playerObject->transform.translate);
                }
            }

            particleSystem_.Update(deltaTime);
        }

        void OnDraw() override {
            particleSystem_.Draw(
                GetRenderer(),
                GetActiveCamera());
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
        MTEngine::ParticleSystem::EmitterId playerEmitterId_ = 0;
        Player player_;
        GameCamera gameCamera_;
        Clock::time_point previousFrameTime_{};
        bool isGameCameraActive_ = true;

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
