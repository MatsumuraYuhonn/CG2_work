#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "MTEngine/Engine/Math/Vector.h" 
#include "MTEngine/Engine/Graphics/TextureManager.h"

namespace MTEngine {

    struct VertexData {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    struct MaterialData {
        std::string textureFilePath;
    };

    struct MeshData {
        std::vector<VertexData> vertices;
        MaterialData material;
    };

    struct ModelData {
        std::vector<MeshData> meshes;
    };

    // 前方宣言 (TextureManagerの実装に合わせて適宜調整してください)
    class TextureManager;

    class Model {
    public:
        static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

        Model();
        ~Model();

        bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, const ModelData& modelData);
        void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, TextureManager* textureManager);

        struct MeshResource {
            Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
            D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
            std::string textureFilePath;
            UINT vertexCount; // 頂点数を保持
        };

        std::vector<MeshResource> meshResources_;

    private:
        // mtlファイル内の全マテリアルを name -> MaterialData で返す（マルチマテリアル対応）
        static std::unordered_map<std::string, MaterialData> LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
        Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes);

        ModelData modelData_;
    };
}