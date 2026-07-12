#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <string>
#include "MTEngine/Engine/Math/Vector.h" 

// 頂点データ構造
namespace MTEngine {

    struct VertexData {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    // マテリアルデータ
    struct MaterialData {
        std::string textureFilePath;
    };

    // モデルデータ全体
    struct ModelData {
        std::vector<VertexData> vertices;
        MaterialData material;
    };

}


// 3Dモデルリソースの管理と描画を行うクラス
namespace MTEngine {

    class Model {
    public:
        // OBJファイルの読み込み
        static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

        Model();
        ~Model();

        // 頂点バッファの初期化
        bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, const ModelData& modelData);

        // 描画実行
        void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

        // テクスチャパスの取得
        const std::string& GetTextureFilePath() const { return modelData_.material.textureFilePath; }

    private:
        // マテリアルファイルの読み込み
        static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

        // バッファリソースの生成
        Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes);

        ModelData modelData_;
        Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    };

}