#pragma once

#include <d3d12.h>
#include <cstdint>

namespace MTEngine {

    class GameScene;
    class DebugCamera;

    class Editor {
    public:
        void Initialize(
            D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle,
            GameScene* gameScene,
            DebugCamera* debugCamera);
        void Update();
        int32_t GetSelectedTileIndex() const { return selectedTileIndex_; }
        bool IsSceneViewFocused() const { return isSceneViewFocused_; }

    private:
        D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle_{};
        GameScene* gameScene_ = nullptr;
        DebugCamera* debugCamera_ = nullptr;
        int32_t selectedTileIndex_ = -1;
        bool isDebugCameraSelected_ = false;
        bool isPlayerSelected_ = false;
        bool isSceneViewFocused_ = false;
        bool isLayoutInitialized_ = false;
    };

}
