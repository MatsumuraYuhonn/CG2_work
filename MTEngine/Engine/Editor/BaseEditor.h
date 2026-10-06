#pragma once

#include <d3d12.h>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace MTEngine {

    class DebugCamera;

    // ゲーム固有クラスに依存しない、MTEngine 共通のエディターUI。
    class BaseEditor {
    public:
        void Initialize(
            D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle,
            DebugCamera* debugCamera);
        void Update();

        // ゲーム固有オブジェクトをHierarchyとInspectorへ登録する。
        void RegisterGameObject(
            const std::string& name,
            std::function<void()> drawInspector);

        bool IsSceneViewFocused() const { return isSceneViewFocused_; }

    private:
        struct GameObjectEntry {
            std::string name;
            std::function<void()> drawInspector;
        };

        D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle_{};
        DebugCamera* debugCamera_ = nullptr;
        bool isSceneViewFocused_ = false;
        bool isLayoutInitialized_ = false;
        size_t selectedObjectIndex_ = 0;
        std::vector<GameObjectEntry> gameObjects_;
    };

}
