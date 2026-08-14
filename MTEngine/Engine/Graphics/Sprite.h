#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "MTEngine/Engine/Math/Vector.h"
#include "MTEngine/Engine/Math/Transform.h"
#include "MTEngine/Engine/Math/Matrix.h"

// スプライトの描画に必要なマテリアル定数バッファ用構造体
namespace MTEngine {

    struct Material {
        Vector4 color;              // スプライトの色情報
        int32_t enabledLighting;    // ライティングの有効/無効フラグ
        float padding[3];           // アライメント用パディング
        Matrix4x4 uvTransform;      // UV変換行列
        int32_t lightingMode;
        int32_t useTexture;
        int32_t isSelected;
        float padding3[3];
    };

}


// 2Dスプライト描画クラス
namespace MTEngine {

    class Sprite {
    public:
        // 初期化：リソース作成やバッファビューの構築を行う
        void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, uint32_t width, uint32_t height, D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU);

        // 更新処理：座標変換行列の転送など
        void Update(const Matrix4x4& projectionMatrix);

        // 描画処理：コマンドリストへのバインドとドローコール
        // commandList: 使用するグラフィックスコマンドリスト
        // directionalLightResource: ライティング用定数バッファリソース
        void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, ID3D12Resource* directionalLightResource);

    public:
        Transform transform{};      // ワールド変換
        Transform uvTransform{};    // UV変換
        Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 頂点カラー

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

}
