#include "Model.h"
#include <fstream>
#include <sstream>
#include <cassert>

namespace MTEngine {

    ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
        ModelData modelData;
        modelData.meshes.emplace_back(); // 初期メッシュ作成

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

            } else if (identifier == "vt") {

                Vector2 texcoord;
                s >> texcoord.x >> texcoord.y;
                texcoord.y = 1.0f - texcoord.y;
                texcoords.push_back(texcoord);

            }else if (identifier == "vn") {

                Vector3 normal;
                s >> normal.x >> normal.y >> normal.z;
                normals.push_back(normal);

            }else if (identifier == "f") {

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
                // 反転させず、読み込んだ順のまま格納
                modelData.meshes.back().vertices.push_back(triangle[2]);
                modelData.meshes.back().vertices.push_back(triangle[1]);
                modelData.meshes.back().vertices.push_back(triangle[0]);

            } else if (identifier == "mtllib") {
                std::string materialFilename;

                s >> materialFilename;
                modelData.meshes.back().material = Model::LoadMaterialTemplateFile(directoryPath, materialFilename);
            }
        }
        return modelData;
    }

    Model::Model() {}
    Model::~Model() {}

    MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
        MaterialData materialData;
        std::string line;
        std::ifstream file(directoryPath + "/" + filename);
        if (!file.is_open()) return materialData;

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

    bool Model::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, const ModelData& modelData) {
        meshResources_.clear();
        for (const auto& meshData : modelData.meshes) {
            if (meshData.vertices.empty()) continue;

            MeshResource resource;
            resource.vertexResource = CreateBufferResource(device, sizeof(VertexData) * meshData.vertices.size());
            resource.vertexBufferView.BufferLocation = resource.vertexResource->GetGPUVirtualAddress();
            resource.vertexBufferView.SizeInBytes = (UINT)(sizeof(VertexData) * meshData.vertices.size());
            resource.vertexBufferView.StrideInBytes = sizeof(VertexData);
            resource.textureFilePath = meshData.material.textureFilePath;
            resource.vertexCount = (UINT)meshData.vertices.size();

            VertexData* vertexData = nullptr;
            resource.vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
            std::memcpy(vertexData, meshData.vertices.data(), sizeof(VertexData) * meshData.vertices.size());
            resource.vertexResource->Unmap(0, nullptr);

            meshResources_.push_back(resource);
        }
        return true;
    }

    void Model::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, TextureManager* textureManager) {
        for (const auto& mesh : meshResources_) {
            if (textureManager) {
                D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = textureManager->GetGPUDescriptorHandle(mesh.textureFilePath);
                commandList->SetGraphicsRootDescriptorTable(3, srvHandle);
            }
            commandList->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);
            commandList->DrawInstanced(mesh.vertexCount, 1, 0, 0);
        }
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
        HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&bufferResource));
        assert(SUCCEEDED(hr));
        return bufferResource;
    }
}