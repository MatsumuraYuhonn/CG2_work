#include "Model.h"
#include <fstream>
#include <sstream>
#include <cassert>

namespace MTEngine {

    ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
        ModelData modelData;

        std::vector<Vector4> positions;
        std::vector<Vector3> normals;
        std::vector<Vector2> texcoords;
        std::string line;

        std::ifstream file(directoryPath + "/" + filename);
        assert(file.is_open());

        // mtlファイルに定義されている全マテリアル（name -> MaterialData）
        std::unordered_map<std::string, MaterialData> materials;
        // マテリアル名 -> modelData.meshes内のインデックス（同じマテリアルの面は同じメッシュにまとめる）
        std::unordered_map<std::string, size_t> materialToMeshIndex;
        std::string currentMaterialName; // "" = マテリアル未指定

        auto GetOrCreateMesh = [&](const std::string& materialName) -> MeshData& {
            auto it = materialToMeshIndex.find(materialName);
            if (it != materialToMeshIndex.end()) {
                return modelData.meshes[it->second];
            }
            modelData.meshes.emplace_back();
            size_t index = modelData.meshes.size() - 1;
            materialToMeshIndex[materialName] = index;
            auto matIt = materials.find(materialName);
            if (matIt != materials.end()) {
                modelData.meshes[index].material = matIt->second;
            }
            return modelData.meshes[index];
            };

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
            else if (identifier == "usemtl") {

                s >> currentMaterialName;
                // マテリアル切り替え時点でメッシュを確保しておく
                GetOrCreateMesh(currentMaterialName);

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
                // 現在のマテリアルに対応するメッシュへ、読み込んだ順のまま格納
                MeshData& mesh = GetOrCreateMesh(currentMaterialName);
                mesh.vertices.push_back(triangle[2]);
                mesh.vertices.push_back(triangle[1]);
                mesh.vertices.push_back(triangle[0]);

            }
            else if (identifier == "mtllib") {
                std::string materialFilename;

                s >> materialFilename;
                materials = Model::LoadMaterialTemplateFile(directoryPath, materialFilename);

                // 既に生成済みのメッシュがあれば、対応するマテリアルを反映する
                for (auto& [name, index] : materialToMeshIndex) {
                    auto matIt = materials.find(name);
                    if (matIt != materials.end()) {
                        modelData.meshes[index].material = matIt->second;
                    }
                }
            }
        }

        // usemtl/mtllibが一度も無いobjへの後方互換（空メッシュを1つ用意）
        if (modelData.meshes.empty()) {
            modelData.meshes.emplace_back();
        }

        return modelData;
    }

    Model::Model() {}
    Model::~Model() {}

    std::unordered_map<std::string, MaterialData> Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
        std::unordered_map<std::string, MaterialData> materials;
        std::string line;
        std::ifstream file(directoryPath + "/" + filename);
        if (!file.is_open()) return materials;

        std::string currentName;
        while (std::getline(file, line)) {
            std::string identifier;
            std::istringstream s(line);
            s >> identifier;
            if (identifier == "newmtl") {
                s >> currentName;
                materials[currentName] = MaterialData{};
            }
            else if (identifier == "map_Kd") {
                std::string textureFilename;
                s >> textureFilename;
                if (!currentName.empty()) {
                    materials[currentName].textureFilePath = directoryPath + "/" + textureFilename;
                }
            }
        }
        return materials;
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