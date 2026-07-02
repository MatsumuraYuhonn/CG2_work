#include "Model.h"
#include <fstream>
#include <sstream>
#include <cassert>

ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& filename) {

    ModelData modelData;
    std::vector<Vector4> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> texcoords;
    std::string line;

    std::ifstream file(directoryPath + "/" + filename);
    assert(file.is_open());

    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        if (identifier == "v") {
            Vector4 position;
            s >> position.x >> position.y >> position.z;
            position.w = 1.0f;
            positions.push_back(position);
        }
        else if (identifier == "vt") {
            Vector2 texcoord;
            s >> texcoord.x >> texcoord.y;
            texcoord.y = 1.0f - texcoord.y;
            texcoords.push_back(texcoord);
        }
        else if (identifier == "vn") {
            Vector3 normal;
            s >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (identifier == "f") {
            VertexData triangle[3];
            for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
                std::string vertexDefinition;
                s >> vertexDefinition;

                std::istringstream v(vertexDefinition);
                uint32_t elementIndices[3];
                for (int32_t element = 0; element < 3; ++element) {
                    std::string index;
                    std::getline(v, index, '/');
                    elementIndices[element] = std::stoi(index);
                }

                Vector4 position = positions[elementIndices[0] - 1];
                Vector2 texcoord = texcoords[elementIndices[1] - 1];
                Vector3 normal = normals[elementIndices[2] - 1];

                position.x *= -1.0f;
                normal.x *= -1.0f;

                triangle[faceVertex] = { position, texcoord, normal };
            }
            modelData.vertices.push_back(triangle[2]);
            modelData.vertices.push_back(triangle[1]);
            modelData.vertices.push_back(triangle[0]);
        }
        else if (identifier == "mtllib") {
            std::string materialFilename;
            s >> materialFilename;
            modelData.material = Model::LoadMaterialTemplateFile(directoryPath, materialFilename);
        }
    }
    return modelData;
}

Model::Model()
{
}

Model::~Model()
{
}

MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
    MaterialData materialData;
    std::string line;
    std::ifstream file(directoryPath + "/" + filename);
    assert(file.is_open());

    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        if (identifier == "map_Kd") {
            std::string textureFilename;
            s >> textureFilename;
            materialData.textureFilePath = directoryPath + "/" + textureFilename;
        }
    }
    return materialData;
}

// --- 初期化・リソース作成 ---
bool Model::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, const ModelData& modelData) {
    modelData_ = modelData;

    // 頂点リソースの作成
    vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * modelData_.vertices.size());

    // ビューの設定
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    // データの転送（Map）
    VertexData* vertexData = nullptr;
    HRESULT hr = vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
    if (FAILED(hr)) return false;

    std::memcpy(vertexData, modelData_.vertices.data(), sizeof(VertexData) * modelData_.vertices.size());
    vertexResource_->Unmap(0, nullptr);

    return true;
}

// --- 描画処理 ---
void Model::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->DrawInstanced(UINT(modelData_.vertices.size()), 1, 0, 0);
}

Microsoft::WRL::ComPtr<ID3D12Resource> Model::CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes) {
    D3D12_HEAP_PROPERTIES uploadHeapProperties{};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferResourceDesc{};
    bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferResourceDesc.Width = sizeInBytes;
    bufferResourceDesc.Height = 1;
    bufferResourceDesc.DepthOrArraySize = 1;
    bufferResourceDesc.MipLevels = 1;
    bufferResourceDesc.SampleDesc.Count = 1;
    bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&bufferResource)
    );
    assert(SUCCEEDED(hr));

    return bufferResource;
}