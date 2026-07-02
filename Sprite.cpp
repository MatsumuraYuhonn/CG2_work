#include "Sprite.h"
#include <cassert>
#include "Model.h"
#include "Matrix.h"

extern Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes);
extern Matrix4x4 MakeIdentityMatrix();
extern Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);
// ...（必要な行列関数のextern宣言、またはヘッダのinclude）

void Sprite::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, uint32_t width, uint32_t height, D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU) {
    width_ = width;
    height_ = height;
    textureSrvHandleGPU_ = textureSrvHandleGPU;

    // 初期トランスフォームの設定
    transform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    uvTransform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

    // --- 頂点リソースの作成とデータ書き込み ---
    vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * 4);
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    // main.cppにあった頂点座標の設定をここで行う（width_, height_ を使用して可変にすると便利）
    vertexData[0].position = { 0.0f, float(height_), 0.0f, 1.0f }; // ...以下略
    vertexResource_->Unmap(0, nullptr);

    // --- インデックスリソースの作成とデータ書き込み ---
    indexResource_ = CreateBufferResource(device, sizeof(uint32_t) * 6);
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    uint32_t* indexData = nullptr;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
    indexData[0] = 0; indexData[1] = 1; indexData[2] = 2;
    indexData[3] = 1; indexData[4] = 3; indexData[5] = 2;
    indexResource_->Unmap(0, nullptr);

    // --- 各種定数バッファの作成 ---
    materialResource_ = CreateBufferResource(device, (sizeof(Material) + 255) & ~255);
    transformationMatrixResource_ = CreateBufferResource(device, (sizeof(Matrix4x4) + 255) & ~255);
}

void Sprite::Update(const Matrix4x4& projectionMatrix) {
    // 1. マテリアル（色・UV）の更新
    Material* materialData = nullptr;
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
    materialData->color = color;
    materialData->enabledLighting = false;
    // uvTransform の計算をして代入
    materialResource_->Unmap(0, nullptr);

    // 2. 行列（WVP）の更新
    Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
    Matrix4x4 wvpMatrix = Multiply(worldMatrix, projectionMatrix); // 2DなのでViewは単位行列でOK

    Matrix4x4* wvpData = nullptr;
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
    *wvpData = wvpMatrix;
    transformationMatrixResource_->Unmap(0, nullptr);
}

void Sprite::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, ID3D12Resource* directionalLightResource) {
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetIndexBuffer(&indexBufferView_);

    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU_);
    commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}