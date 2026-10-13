#include <memory>

#include "MTEngine/Engine/Base/Engine.h"
#include "MTEngine/Engine/Scene/SceneObject.h"
#include "MTEngine/Engine/Scene/SceneSerializer.h"

namespace {

    class GameApplication final : public MTEngine::Engine {
    protected:
        void OnInitialize() override {
            // JSONがない場合の初期値
            fenceObject_.id = 1;
            fenceObject_.name = "Fence";
            fenceObject_.modelPath =
                "MTEngine/Assets/Resources/fence/fence.obj";

            fenceObject_.transform.translate = {
                0.0f, 0.0f, 0.0f
            };

            fenceObject_.transform.rotate = {
                0.0f, 0.0f, 0.0f
            };

            fenceObject_.transform.scale = {
                1.0f, 1.0f, 1.0f
            };

            const bool loaded =
                MTEngine::SceneSerializer::Load(
                    kSceneFilePath,
                    fenceObject_);

            // 初回起動ならJSONを作る
            if (!loaded) {
                MTEngine::SceneSerializer::Save(
                    kSceneFilePath,
                    fenceObject_);
            }
        }

        void OnUpdate() override {
        }

        void OnDraw() override {
        }

        void OnFinalize() override {
            MTEngine::SceneSerializer::Save(
                kSceneFilePath,
                fenceObject_);
        }

    private:
        static constexpr const char* kSceneFilePath =
            "MTEngine/Assets/Scenes/main.scene.json";

        MTEngine::SceneObject fenceObject_;
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