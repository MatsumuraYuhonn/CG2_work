#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <unordered_map>
#include "externals/DirectXTex/DirectXTex.h"

// 前方宣言（循環参照防止）
class DescriptorHeapManager;

class TextureManager {
public:
    struct TextureData {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        DirectX::TexMetadata metadata;
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
    };

    // シングルトンにするか、通常のインスタンスにするかはお好みで。ここでは通常クラスとして定義
    void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, DescriptorHeapManager* srvHeapManager);

    // テクスチャを読み込んでSRVまで作成する
    void Load(const std::string& filePath, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

    // GPUハンドルを取得する
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const std::string& filePath) const;

private:
    // main.cpp から移植する内部関数
    DirectX::ScratchImage LoadTextureFile(const std::string& filePath);
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);
    [[nodiscard]]
    Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(
        Microsoft::WRL::ComPtr<ID3D12Resource> texture,
        const DirectX::ScratchImage& mipImages,
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

private:
    Microsoft::WRL::ComPtr<ID3D12Device> device_ = nullptr;
    DescriptorHeapManager* srvHeapManager_ = nullptr;

    // ファイルパスをキーにしてテクスチャデータを管理
    std::unordered_map<std::string, TextureData> textures_;

    // 中間リソースが消えないようにループの最後まで保持する用（必要に応じてリセットする仕組みにしてもOK）
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> intermediateResources_;
};
