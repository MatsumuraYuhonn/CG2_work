#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "externals/DirectXTex/DirectXTex.h"

namespace MTEngine {

    class DescriptorHeapManager;

}

// テクスチャリソースおよびSRVを管理するクラス
namespace MTEngine {

    class TextureManager {
    public:
        struct TextureData {
            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            DirectX::TexMetadata metadata;
            D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
            D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
        };

        // 初期化
        void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, DescriptorHeapManager* srvHeapManager);

        // ファイルからテクスチャを読み込み、リソースおよびSRVを作成する
        // filePath: 読み込むファイルのパス
        // commandList: コピー転送に使用するコマンドリスト
        void Load(const std::string& filePath, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

        // 指定したパスのテクスチャに対応するGPUディスクリプタハンドルを取得
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const std::string& filePath) const;

    private:
        // テクスチャファイルをロード
        DirectX::ScratchImage LoadTextureFile(const std::string& filePath);

        // テクスチャ用GPUリソースの生成
        Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

        // テクスチャデータをCPUからGPUへアップロードする
        // 戻り値: 転送完了まで生存させる中間リソース
        [[nodiscard]]
        Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(
            Microsoft::WRL::ComPtr<ID3D12Resource> texture,
            const DirectX::ScratchImage& mipImages,
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

    private:
        Microsoft::WRL::ComPtr<ID3D12Device> device_ = nullptr;
        DescriptorHeapManager* srvHeapManager_ = nullptr;

        // ファイルパスをキーにしたテクスチャ管理マップ
        std::unordered_map<std::string, TextureData> textures_;

        // アップロード用中間リソースの保持（生存期間管理）
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> intermediateResources_;
    };

}