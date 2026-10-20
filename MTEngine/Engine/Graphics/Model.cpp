#include "Model.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <utility>

namespace MTEngine {

    std::optional<ModelData> Model::LoadObjFile(
        const std::string& directoryPath,
        const std::string& filename) {

        ModelData modelData;

        std::vector<Vector4> positions;
        std::vector<Vector3> normals;
        std::vector<Vector2> texcoords;
        std::string line;

        const std::string filePath = directoryPath + "/" + filename;
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return std::nullopt;
        }

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

                std::vector<VertexData> faceVertices;
                std::string vertexDefinition;
                while (s >> vertexDefinition) {
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
                    faceVertices.push_back({ position, texcoord, normal });
                }
                // 現在のマテリアルに対応するメッシュへ、読み込んだ順のまま格納
                MeshData& mesh = GetOrCreateMesh(currentMaterialName);
                // Triangulate quads and other polygon faces. Reversing the winding
                // preserves the existing coordinate-system conversion.
                for (size_t faceVertex = 1; faceVertex + 1 < faceVertices.size(); ++faceVertex) {
                    mesh.vertices.push_back(faceVertices[faceVertex + 1]);
                    mesh.vertices.push_back(faceVertices[faceVertex]);
                    mesh.vertices.push_back(faceVertices[0]);
                }

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

        // ファイルを開けても、面がなく描画可能な頂点が無ければ読み込み失敗とする。
        bool hasVertices = false;
        for (const MeshData& mesh : modelData.meshes) {
            if (!mesh.vertices.empty()) {
                hasVertices = true;
                break;
            }
        }

        if (!hasVertices) {
            return std::nullopt;
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
                std::getline(s >> std::ws, textureFilename);
                if (!currentName.empty()) {
                    // Blender may export an absolute path from the creator's PC.
                    // Prefer a texture with the same filename beside the model so
                    // re-exporting the MTL does not require manual path edits.
                    // Do this with byte strings because std::filesystem::path can
                    // throw when an exported path contains characters outside the
                    // active Windows code page.
                    const size_t filenameStart = textureFilename.find_last_of("/\\");
                    const std::string textureBasename = filenameStart == std::string::npos
                        ? textureFilename
                        : textureFilename.substr(filenameStart + 1);
                    const std::string localTexturePath =
                        directoryPath + "/" + textureBasename;
                    std::ifstream localTextureFile(localTexturePath, std::ios::binary);
                    if (localTextureFile.good()) {
                        materials[currentName].textureFilePath = localTexturePath;
                    }
                    else {
                        const bool isAbsolutePath =
                            (textureFilename.size() >= 2 && textureFilename[1] == ':') ||
                            (!textureFilename.empty() &&
                                (textureFilename[0] == '/' || textureFilename[0] == '\\'));
                        if (!isAbsolutePath) {
                            materials[currentName].textureFilePath =
                                directoryPath + "/" + textureFilename;
                        }
                        else {
                            materials[currentName].textureFilePath = textureFilename;
                        }
                    }
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

            resource.materialBuffer =
                std::make_unique<MaterialConstantBuffer>();
            if (!resource.materialBuffer->Initialize(device)) {
                meshResources_.clear();
                return false;
            }

            (*resource.materialBuffer)->color =
                { 1.0f, 1.0f, 1.0f, 1.0f };
            (*resource.materialBuffer)->enabledLighting = 0;
            (*resource.materialBuffer)->uvTransform = MakeIdentityMatrix();
            (*resource.materialBuffer)->lightingMode = 0;
            (*resource.materialBuffer)->useTexture = 0;
            (*resource.materialBuffer)->isSelected = 0;

            VertexData* vertexData = nullptr;
            resource.vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
            std::memcpy(vertexData, meshData.vertices.data(), sizeof(VertexData) * meshData.vertices.size());
            resource.vertexResource->Unmap(0, nullptr);

            meshResources_.push_back(std::move(resource));
        }
        return true;
    }

    void Model::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, TextureManager* textureManager) {
        for (const auto& mesh : meshResources_) {
            if (textureManager && !mesh.textureFilePath.empty()) {
                D3D12_GPU_DESCRIPTOR_HANDLE srvHandle{};
                if (textureManager->TryGetGPUDescriptorHandle(
                    mesh.textureFilePath,
                    srvHandle)) {

                    commandList->SetGraphicsRootDescriptorTable(3, srvHandle);
                }
            }
            commandList->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);
            commandList->DrawInstanced(mesh.vertexCount, 1, 0, 0);
        }
    }

    void Model::DrawTextured(
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
        TextureManager* textureManager) {

        for (auto& mesh : meshResources_) {
            if (!mesh.materialBuffer) {
                continue;
            }

            int32_t useTexture = 0;
            D3D12_GPU_DESCRIPTOR_HANDLE srvHandle{};
            if (textureManager && !mesh.textureFilePath.empty()) {
                if (textureManager->TryGetGPUDescriptorHandle(
                    mesh.textureFilePath,
                    srvHandle)) {

                    useTexture = 1;
                }
            }

            (*mesh.materialBuffer)->useTexture = useTexture;
            commandList->SetGraphicsRootConstantBufferView(
                0,
                mesh.materialBuffer->GetGPUVirtualAddress());

            if (useTexture != 0) {
                commandList->SetGraphicsRootDescriptorTable(3, srvHandle);
            }

            commandList->IASetVertexBuffers(
                0,
                1,
                &mesh.vertexBufferView);
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
