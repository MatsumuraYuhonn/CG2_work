#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "Vector.h"
#include "Transform.h"
#include "Matrix.h"

struct Material {
    Vector4 color;
    int32_t enabledLighting;
    float padding[3];
    Matrix4x4 uvTransform;
};


class Sprite {
public:
    void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, uint32_t width, uint32_t height, D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU);
    void Update(const Matrix4x4& projectionMatrix);
    void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, ID3D12Resource* directionalLightResource);

public:
    Transform transform{};
    Transform uvTransform{};
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;

    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};
    uint32_t width_ = 0;
    uint32_t height_ = 0;
};