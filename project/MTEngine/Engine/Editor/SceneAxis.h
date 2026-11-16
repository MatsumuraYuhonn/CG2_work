#pragma once

#include <cstdint>
#include <array>
#include <d3d12.h>
#include <wrl.h>

#include "MTEngine/Engine/Graphics/ConstantBuffer.h"
#include "MTEngine/Engine/Graphics/Model.h"
#include "MTEngine/Engine/Graphics/TextureManager.h"
#include "MTEngine/Engine/Math/Matrix.h"

namespace MTEngine {

    // Renders axis.obj in the upper-right corner of the Scene viewport.
    class SceneAxis {
    public:
        void Initialize(
            Microsoft::WRL::ComPtr<ID3D12Device> device,
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
            DescriptorHeapManager* srvHeapManager);
        void SetViewMatrix(const Matrix4x4& viewMatrix);
        void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, int32_t sceneWidth, int32_t sceneHeight);

    private:
        std::array<ModelData, 3> axisModelData_;
        std::array<Model, 3> axisModels_;
        TextureManager textureManager_;
        std::array<MaterialConstantBuffer, 3> materialConstantBuffers_;
        TransformationMatrixConstantBuffer transformConstantBuffer_;
        DirectionalLightConstantBuffer lightConstantBuffer_;
    };

}
