#include <memory>

#include "MTEngine/Engine/Base/Engine.h"
#include "MTEngine/Engine/Scene/SceneObject.h"
#include "MTEngine/Engine/Scene/SceneSerializer.h"
#include "MTEngine/Engine/Scene/Scene.h"

namespace {

    class GameApplication final : public MTEngine::Engine {
    protected:
        void OnInitialize() override {
            const bool loaded =
                MTEngine::SceneSerializer::Load(
                    kSceneFilePath,
                    GetScene());
        }

        void OnFinalize() override {
            MTEngine::SceneSerializer::Save(
                kSceneFilePath,
                GetScene());
        }

    private:
        static constexpr const char* kSceneFilePath =
            "MTEngine/Assets/Scenes/main.scene.json";
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