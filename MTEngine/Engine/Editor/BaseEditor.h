#pragma once

#include <d3d12.h>

namespace MTEngine {

    class DebugCamera;

    // ゲーム固有クラスに依存しない、MTEngine 共通のエディターUI。
    class BaseEditor {
    public:
        void Initialize(
            D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle,
            DebugCamera* debugCamera);
        void Update();

        bool IsSceneViewFocused() const { return isSceneViewFocused_; }

    private:
        D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle_{};
        DebugCamera* debugCamera_ = nullptr;
        bool isSceneViewFocused_ = false;
        bool isDebugCameraSelected_ = true;
        bool isLayoutInitialized_ = false;
    };

}
