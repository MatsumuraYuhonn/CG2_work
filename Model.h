#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <string>
#include "Vector.h" 

struct VertexData {
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
};

struct MaterialData {
    std::string textureFilePath;
};

struct ModelData {
    std::vector<VertexData> vertices;
    MaterialData material;
};

class Model {
public:

    static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

    Model();
    ~Model();

    bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, const ModelData& modelData);

    // 描画
    void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

    const std::string& GetTextureFilePath() const { return modelData_.material.textureFilePath; }

private:

    static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes);


    ModelData modelData_;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

};