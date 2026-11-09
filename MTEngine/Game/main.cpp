#include <chrono>
#include <memory>

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
                });

            previousFrameTime_ = Clock::now();
        }

        void OnUpdate() override {
            const Clock::time_point currentTime = Clock::now();
            const std::chrono::duration<float> elapsed =
                currentTime - previousFrameTime_;
            previousFrameTime_ = currentTime;

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
