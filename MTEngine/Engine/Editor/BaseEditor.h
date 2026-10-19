#pragma once

#include <d3d12.h>
#include <cstdint>

namespace MTEngine {

    class DebugCamera;
    class Scene;

    // ゲーム固有クラスに依存しない、MTEngine 共通のエディターUI。
    class BaseEditor {
    public:
        void Initialize(
            D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle,
            DebugCamera* debugCamera, Scene* scene);
        void Update();

        bool IsSceneViewFocused() const { return isSceneViewFocused_; }

    private:
        D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle_{};
        
        DebugCamera* debugCamera_ = nullptr;
        Scene* scene_ = nullptr;

        uint64_t selectedObjectId_ = 0;

        bool isSceneViewFocused_ = false;
        bool isDebugCameraSelected_ = true;
        bool isLayoutInitialized_ = false;

        char newObjectName_[128] = "New Object";
        char newModelPath_[512] = "";

    };

}
