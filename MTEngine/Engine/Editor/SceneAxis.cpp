#include "SceneAxis.h"

#include "MTEngine/Engine/Math/Matrix.h"

#include <cassert>
#include <cmath>

namespace MTEngine {

    namespace {

        Matrix4x4 MakeAxisProjectionMatrix() {
            // The longest axis is four units.  Keep that radius centered in
            // the viewport so all axes remain visible at every orientation.
            Matrix4x4 projection{};
            projection.m[0][0] = 1.0f / 4.5f;
            projection.m[1][1] = 1.0f / 4.5f;
            projection.m[2][2] = 1.0f / 10.0f;
            projection.m[3][2] = 0.0f;
            projection.m[3][3] = 1.0f;
            return projection;
        }

        size_t GetAxisIndex(const VertexData* triangle) {
            float xExtent = 0.0f;
            float yExtent = 0.0f;
            float zExtent = 0.0f;
            for (size_t vertexIndex = 0; vertexIndex < 3; ++vertexIndex) {
                const Vector4& position = triangle[vertexIndex].position;
                const float x = std::fabs(position.x);
                const float y = std::fabs(position.y);
                const float z = std::fabs(position.z);
                xExtent = x > xExtent ? x : xExtent;
                yExtent = y > yExtent ? y : yExtent;
                zExtent = z > zExtent ? z : zExtent;
            }

            if (yExtent > xExtent && yExtent >= zExtent) {
                return 1;
            }
            if (zExtent > xExtent && zExtent > yExtent) {
                return 2;
            }
            return 0;
        }

        void SplitAxisModel(const ModelData& source, std::array<ModelData, 3>& destination) {
            for (const MeshData& sourceMesh : source.meshes) {
                for (ModelData& axisModelData : destination) {
                    axisModelData.meshes.emplace_back();
                    axisModelData.meshes.back().material = sourceMesh.material;
                }

                for (size_t vertexIndex = 0; vertexIndex + 2 < sourceMesh.vertices.size(); vertexIndex += 3) {
                    const VertexData* triangle = &sourceMesh.vertices[vertexIndex];
                    const size_t axisIndex = GetAxisIndex(triangle);
                    std::vector<VertexData>& vertices = destination[axisIndex].meshes.back().vertices;
                    vertices.insert(vertices.end(), triangle, triangle + 3);
                }
            }
        }

    }

    void SceneAxis::Initialize(
        Microsoft::WRL::ComPtr<ID3D12Device> device,
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
        DescriptorHeapManager* srvHeapManager)
    {
        assert(device);
        assert(commandList);
        assert(srvHeapManager);

        transformConstantBuffer_.Initialize(device);
        lightConstantBuffer_.Initialize(device);

        const ModelData sourceModelData = Model::LoadObjFile("MTEngine/Assets/Resources", "axis.obj");
        SplitAxisModel(sourceModelData, axisModelData_);
        for (size_t axisIndex = 0; axisIndex < axisModels_.size(); ++axisIndex) {
            axisModels_[axisIndex].Initialize(device, axisModelData_[axisIndex]);
        }

        textureManager_.Initialize(device, srvHeapManager);
        for (const ModelData& axisModelData : axisModelData_) {
            for (const MeshData& mesh : axisModelData.meshes) {
                if (!mesh.material.textureFilePath.empty()) {
                    textureManager_.Load(mesh.material.textureFilePath, commandList);
                }
            }
        }

        const Vector4 axisColors[] = {
            { 0.94f, 0.20f, 0.20f, 1.0f },
            { 0.25f, 0.82f, 0.34f, 1.0f },
            { 0.20f, 0.48f, 0.94f, 1.0f },
        };
        for (size_t axisIndex = 0; axisIndex < materialConstantBuffers_.size(); ++axisIndex) {
            materialConstantBuffers_[axisIndex].Initialize(device);
            materialConstantBuffers_[axisIndex]->color = axisColors[axisIndex];
            materialConstantBuffers_[axisIndex]->enabledLighting = 0;
            materialConstantBuffers_[axisIndex]->uvTransform = MakeIdentityMatrix();
            materialConstantBuffers_[axisIndex]->lightingMode = 0;
            materialConstantBuffers_[axisIndex]->useTexture = 0;
            materialConstantBuffers_[axisIndex]->isSelected = 0;
        }

        lightConstantBuffer_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        lightConstantBuffer_->direction = { 0.0f, -1.0f, 1.0f };
        lightConstantBuffer_->intensity = 1.0f;

        const Matrix4x4 worldMatrix = MakeTranslateMatrix({ 0.0f, 0.0f, 5.0f });
        transformConstantBuffer_->World = worldMatrix;
        transformConstantBuffer_->WVP = Multiply(worldMatrix, MakeAxisProjectionMatrix());
    }

    void SceneAxis::SetViewMatrix(const Matrix4x4& viewMatrix) {
        Matrix4x4 rotationOnlyView = viewMatrix;
        rotationOnlyView.m[3][0] = 0.0f;
        rotationOnlyView.m[3][1] = 0.0f;
        rotationOnlyView.m[3][2] = 0.0f;
        rotationOnlyView.m[3][3] = 1.0f;

        const Matrix4x4 worldMatrix = MakeTranslateMatrix({ 0.0f, 0.0f, 5.0f });
        transformConstantBuffer_->World = worldMatrix;
        // Keep the depth offset in view space.  Applying it before the view
        // rotation moves the whole gizmo sideways when the camera rotates.
        transformConstantBuffer_->WVP = Multiply(Multiply(rotationOnlyView, worldMatrix), MakeAxisProjectionMatrix());
    }

    void SceneAxis::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, int32_t sceneWidth, int32_t sceneHeight) {
        constexpr int32_t kMargin = 16;
        constexpr int32_t kViewportSize = 64;
        if (sceneWidth <= kMargin * 2 || sceneHeight <= kMargin * 2) {
            return;
        }

        const int32_t availableWidth = sceneWidth - kMargin * 2;
        const int32_t availableHeight = sceneHeight - kMargin * 2;
        int32_t viewportSize = kViewportSize;
        if (viewportSize > availableWidth) {
            viewportSize = availableWidth;
        }
        if (viewportSize > availableHeight) {
            viewportSize = availableHeight;
        }

        D3D12_VIEWPORT axisViewport{};
        axisViewport.TopLeftX = static_cast<float>(sceneWidth - kMargin - viewportSize);
        axisViewport.TopLeftY = static_cast<float>(kMargin);
        axisViewport.Width = static_cast<float>(viewportSize);
        axisViewport.Height = static_cast<float>(viewportSize);
        axisViewport.MinDepth = 0.0f;
        axisViewport.MaxDepth = 1.0f;

        D3D12_RECT axisScissorRect{};
        axisScissorRect.left = static_cast<LONG>(axisViewport.TopLeftX);
        axisScissorRect.top = static_cast<LONG>(axisViewport.TopLeftY);
        axisScissorRect.right = axisScissorRect.left + viewportSize;
        axisScissorRect.bottom = axisScissorRect.top + viewportSize;

        commandList->RSSetViewports(1, &axisViewport);
        commandList->RSSetScissorRects(1, &axisScissorRect);
        commandList->SetGraphicsRootConstantBufferView(1, transformConstantBuffer_.GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(2, lightConstantBuffer_.GetGPUVirtualAddress());
        for (size_t axisIndex = 0; axisIndex < axisModels_.size(); ++axisIndex) {
            commandList->SetGraphicsRootConstantBufferView(0, materialConstantBuffers_[axisIndex].GetGPUVirtualAddress());
            axisModels_[axisIndex].Draw(commandList, &textureManager_);
        }

        D3D12_VIEWPORT sceneViewport{};
        sceneViewport.Width = static_cast<float>(sceneWidth);
        sceneViewport.Height = static_cast<float>(sceneHeight);
        sceneViewport.MinDepth = 0.0f;
        sceneViewport.MaxDepth = 1.0f;
        D3D12_RECT sceneScissorRect{ 0, 0, sceneWidth, sceneHeight };
        commandList->RSSetViewports(1, &sceneViewport);
        commandList->RSSetScissorRects(1, &sceneScissorRect);
    }

}
