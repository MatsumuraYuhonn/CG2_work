#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include "MTEngine/Engine/Graphics/ConstantBuffer.h"
#include "MTEngine/Engine/Graphics/TextureManager.h"

namespace MTEngine {

    class DebugCamera;
    class DescriptorHeapManager;
    class Model;
    class PipelineManager;
    class Scene;
    struct SceneObject;

    // SceneObjectの実行時リソースを準備し、3Dシーンへ描画する。
    class SceneRenderer {
    public:
        bool Initialize(
            Microsoft::WRL::ComPtr<ID3D12Device> device,
            DescriptorHeapManager* srvHeapManager,
            PipelineManager* pipelineManager);

        void Draw(
            Scene& scene,
            const DebugCamera& camera,
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

    private:
        bool LoadModel(
            SceneObject& object,
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

        struct InstanceBuffer {
            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            size_t capacity = 0;
        };

        bool EnsureInstanceBuffer(
            const std::string& modelPath,
            size_t instanceCount);

        Microsoft::WRL::ComPtr<ID3D12Device> device_;
        TextureManager textureManager_;
        DirectionalLightConstantBuffer lightBuffer_;
        PipelineManager* pipelineManager_ = nullptr;
        std::unordered_map<std::string, std::shared_ptr<Model>> modelCache_;
        std::unordered_map<std::string, InstanceBuffer> instanceBuffers_;
        bool isInitialized_ = false;
    };

}
