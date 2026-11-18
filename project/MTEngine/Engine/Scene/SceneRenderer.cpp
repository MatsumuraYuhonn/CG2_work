#include "SceneRenderer.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "MTEngine/Engine/Debug/DebugCamera.h"
#include "MTEngine/Engine/Debug/Logger.h"
#include "MTEngine/Engine/Graphics/PipelineManager.h"
#include "MTEngine/Engine/Math/Matrix.h"
#include "MTEngine/Engine/Scene/Scene.h"

namespace MTEngine {

    namespace {

        Microsoft::WRL::ComPtr<ID3D12Resource> CreateInstanceBufferResource(
            Microsoft::WRL::ComPtr<ID3D12Device> device,
            size_t sizeInBytes) {

            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

            D3D12_RESOURCE_DESC resourceDescription{};
            resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDescription.Width = sizeInBytes;
            resourceDescription.Height = 1;
            resourceDescription.DepthOrArraySize = 1;
            resourceDescription.MipLevels = 1;
            resourceDescription.SampleDesc.Count = 1;
            resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            const HRESULT result = device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDescription,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&resource));

            if (FAILED(result)) {
                return nullptr;
            }

            return resource;
        }

        struct DrawBatch {
            std::string modelPath;
            std::shared_ptr<Model> model;
            std::vector<SceneObject*> objects;
        };

    }

    bool SceneRenderer::Initialize(
        Microsoft::WRL::ComPtr<ID3D12Device> device,
        DescriptorHeapManager* srvHeapManager,
        PipelineManager* pipelineManager) {

        if (!device || !srvHeapManager || !pipelineManager) {
            return false;
        }

        device_ = device;
        pipelineManager_ = pipelineManager;
        textureManager_.Initialize(device_, srvHeapManager);

        if (!lightBuffer_.Initialize(device_)) {
            return false;
        }

        lightBuffer_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        lightBuffer_->direction = { 0.0f, -1.0f, 1.0f };
        lightBuffer_->intensity = 1.0f;

        isInitialized_ = true;
        return true;
    }

    void SceneRenderer::Draw(
        Scene& scene,
        const DebugCamera& camera,
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {

        if (!isInitialized_ || !commandList) {
            return;
        }

        std::vector<DrawBatch> batches;
        std::unordered_map<std::string, size_t> batchIndices;

        for (SceneObject& object : scene.GetObjects()) {
            if (object.modelLoadState == ModelLoadState::Unloaded) {
                LoadModel(object, commandList);
            }

            if (object.modelLoadState != ModelLoadState::Loaded) {
                continue;
            }

            if (!object.model) {
                object.modelLoadState = ModelLoadState::Failed;
                continue;
            }

            const auto batchIterator = batchIndices.find(object.modelPath);
            if (batchIterator != batchIndices.end()) {
                batches[batchIterator->second].objects.push_back(&object);
                continue;
            }

            DrawBatch batch;
            batch.modelPath = object.modelPath;
            batch.model = object.model;
            batch.objects.push_back(&object);

            batchIndices.emplace(object.modelPath, batches.size());
            batches.push_back(std::move(batch));
        }

        pipelineManager_->BindInstanced(
            commandList.Get(),
            BlendMode::kBlendModeNormal);

        commandList->SetGraphicsRootConstantBufferView(
            2,
            lightBuffer_.GetGPUVirtualAddress());

        const Matrix4x4& view = camera.GetViewMatrix();
        const Matrix4x4& projection = camera.GetProjectionMatrix();

        for (DrawBatch& batch : batches) {
            if (batch.objects.empty()) {
                continue;
            }

            if (batch.objects.size() >
                static_cast<size_t>((std::numeric_limits<UINT>::max)())) {

                Log("Too many instances in batch: " + batch.modelPath + "\n");
                continue;
            }

            if (!EnsureInstanceBuffer(
                batch.modelPath,
                batch.objects.size())) {

                Log("Failed to create instance buffer: " +
                    batch.modelPath + "\n");
                continue;
            }

            InstanceBuffer& instanceBuffer =
                instanceBuffers_.at(batch.modelPath);

            void* mappedData = nullptr;
            const D3D12_RANGE readRange{ 0, 0 };
            const HRESULT mapResult = instanceBuffer.resource->Map(
                0,
                &readRange,
                &mappedData);

            if (FAILED(mapResult) || !mappedData) {
                Log("Failed to map instance buffer: " +
                    batch.modelPath + "\n");
                continue;
            }

            TransformationMatrix* transformations =
                static_cast<TransformationMatrix*>(mappedData);

            for (size_t index = 0; index < batch.objects.size(); ++index) {
                const Transform& transform = batch.objects[index]->transform;
                const Matrix4x4 world = MakeAffineMatrix(
                    transform.scale,
                    transform.rotate,
                    transform.translate);
                const Matrix4x4 worldView = Multiply(world, view);

                transformations[index].World = world;
                transformations[index].WVP = Multiply(worldView, projection);
            }

            const D3D12_RANGE writtenRange{
                0,
                sizeof(TransformationMatrix) * batch.objects.size()
            };
            instanceBuffer.resource->Unmap(0, &writtenRange);

            commandList->SetGraphicsRootShaderResourceView(
                4,
                instanceBuffer.resource->GetGPUVirtualAddress());

            batch.model->DrawTexturedInstanced(
                commandList,
                &textureManager_,
                static_cast<UINT>(batch.objects.size()));
        }

        // SceneRenderer以降の通常描画が既存の頂点シェーダーを使えるように戻す。
        pipelineManager_->Bind(
            commandList.Get(),
            BlendMode::kBlendModeNormal,
            DepthWriteMode::kEnable);
    }

    bool SceneRenderer::LoadModel(
        SceneObject& object,
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
        if (object.modelPath.empty()) {
            object.modelLoadState = ModelLoadState::Failed;
            Log("Scene model path is empty: " + object.name + "\n");
            return false;
        }

        const auto cachedModel = modelCache_.find(object.modelPath);
        if (cachedModel != modelCache_.end()) {
            object.model = cachedModel->second;
            object.modelLoadState = ModelLoadState::Loaded;
            return true;
        }

        const size_t separatorPosition =
            object.modelPath.find_last_of("/\\");

        std::string directoryPath = ".";
        std::string filename = object.modelPath;

        if (separatorPosition != std::string::npos) {
            directoryPath = object.modelPath.substr(0, separatorPosition);
            filename = object.modelPath.substr(separatorPosition + 1);
        }

        if (filename.empty()) {
            object.modelLoadState = ModelLoadState::Failed;
            Log("Scene model filename is empty: " + object.modelPath + "\n");
            return false;
        }

        const std::optional<ModelData> modelData =
            Model::LoadObjFile(directoryPath, filename);

        if (!modelData.has_value()) {
            object.modelLoadState = ModelLoadState::Failed;
            Log("Failed to load scene model: " + object.modelPath + "\n");
            return false;
        }

        for (const MeshData& mesh : modelData->meshes) {
            const std::string& textureFilePath =
                mesh.material.textureFilePath;
            if (textureFilePath.empty()) {
                continue;
            }

            if (!textureManager_.Load(textureFilePath, commandList)) {
                Log("Failed to load scene texture: " + textureFilePath + "\n");
            }
        }

        auto model = std::make_shared<Model>();
        if (!model->Initialize(device_, modelData.value())) {
            object.modelLoadState = ModelLoadState::Failed;
            Log("Failed to initialize scene model: " + object.modelPath + "\n");
            return false;
        }

        modelCache_.emplace(object.modelPath, model);
        object.model = std::move(model);
        object.modelLoadState = ModelLoadState::Loaded;
        return true;
    }

    bool SceneRenderer::EnsureInstanceBuffer(
        const std::string& modelPath,
        size_t instanceCount) {

        if (instanceCount == 0) {
            return false;
        }

        InstanceBuffer& instanceBuffer = instanceBuffers_[modelPath];
        if (instanceBuffer.resource &&
            instanceBuffer.capacity >= instanceCount) {

            return true;
        }

        size_t newCapacity = (std::max)(instanceBuffer.capacity, size_t{ 1 });
        while (newCapacity < instanceCount) {
            newCapacity *= 2;
        }

        Microsoft::WRL::ComPtr<ID3D12Resource> resource =
            CreateInstanceBufferResource(
                device_,
                sizeof(TransformationMatrix) * newCapacity);

        if (!resource) {
            return false;
        }

        instanceBuffer.resource = std::move(resource);
        instanceBuffer.capacity = newCapacity;
        return true;
    }

}
