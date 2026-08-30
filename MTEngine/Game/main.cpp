#include <memory>

#include "MTEngine/Engine/Base/Engine.h"

namespace {

    // 新しいゲームは、このクラスの On... 関数へ処理を追加して制作する。
    class GameApplication final : public MTEngine::Engine {
    protected:
        void OnInitialize() override {
            // モデル、テクスチャ、ゲームシーンなどを初期化する。
        }

        void OnUpdate() override {
            // GetInput() を利用してゲームの状態を更新する。
        }

        void OnDraw() override {
            // GetCommandList() と GetDebugCamera() を利用して描画する。
        }

        void OnFinalize() override {
            // ゲーム固有のリソースを解放する。
        }
    };

}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    auto game = std::make_unique<GameApplication>();
    game->Initialize();
    game->Run();
    game->Finalize();
    return 0;
}
