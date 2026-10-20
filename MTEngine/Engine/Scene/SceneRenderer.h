#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "MTEngine/Engine/Graphics/ConstantBuffer.h"
#include "MTEngine/Engine/Graphics/TextureManager.h"

namespace MTEngine {

    class DebugCamera;
    class DescriptorHeapManager;
    class Scene;
    struct SceneObject;

    // SceneObjectの実行時リソースを準備し、3Dシーンへ描画する。
    class SceneRenderer {
    public:
        bool Initialize(
            Microsoft::WRL::ComPtr<ID3D12Device> device,
            DescriptorHeapManager* srvHeapManager);

        void Draw(
            Scene& scene,
            const DebugCamera& camera,
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

    private:
        bool LoadModel(
            SceneObject& object,
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

        Microsoft::WRL::ComPtr<ID3D12Device> device_;
        TextureManager textureManager_;
        DirectionalLightConstantBuffer lightBuffer_;
        bool isInitialized_ = false;
    };

}
