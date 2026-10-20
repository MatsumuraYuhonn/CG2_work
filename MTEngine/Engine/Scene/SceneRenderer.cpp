#include "SceneRenderer.h"

#include <memory>
#include <string>
#include <utility>

#include "MTEngine/Engine/Debug/DebugCamera.h"
#include "MTEngine/Engine/Debug/Logger.h"
#include "MTEngine/Engine/Math/Matrix.h"
#include "MTEngine/Engine/Scene/Scene.h"

namespace MTEngine {

    bool SceneRenderer::Initialize(
        Microsoft::WRL::ComPtr<ID3D12Device> device,
        DescriptorHeapManager* srvHeapManager) {

        if (!device || !srvHeapManager) {
            return false;
        }

        device_ = device;
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

        commandList->SetGraphicsRootConstantBufferView(
            2,
            lightBuffer_.GetGPUVirtualAddress());

        const Matrix4x4& view = camera.GetViewMatrix();
        const Matrix4x4& projection = camera.GetProjectionMatrix();

        for (SceneObject& object : scene.GetObjects()) {
            if (object.modelLoadState == ModelLoadState::Unloaded) {
                LoadModel(object, commandList);
            }

            if (object.modelLoadState != ModelLoadState::Loaded) {
                continue;
            }

            if (!object.model || !object.transformBuffer) {
                object.modelLoadState = ModelLoadState::Failed;
                continue;
            }

            const Matrix4x4 world = MakeAffineMatrix(
                object.transform.scale,
                object.transform.rotate,
                object.transform.translate);
            const Matrix4x4 worldView = Multiply(world, view);
            const Matrix4x4 wvp = Multiply(worldView, projection);

            (*object.transformBuffer)->World = world;
            (*object.transformBuffer)->WVP = wvp;

            commandList->SetGraphicsRootConstantBufferView(
                1,
                object.transformBuffer->GetGPUVirtualAddress());

            object.model->DrawTextured(commandList, &textureManager_);
        }
    }

    bool SceneRenderer::LoadModel(
        SceneObject& object,
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
        if (object.modelPath.empty()) {
            object.modelLoadState = ModelLoadState::Failed;
            Log("Scene model path is empty: " + object.name + "\n");
            return false;
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

        auto transformBuffer =
            std::make_unique<TransformationMatrixConstantBuffer>();
        if (!transformBuffer->Initialize(device_)) {
            object.modelLoadState = ModelLoadState::Failed;
            Log("Failed to initialize transform buffer: " + object.name + "\n");
            return false;
        }

        object.model = std::move(model);
        object.transformBuffer = std::move(transformBuffer);
        object.modelLoadState = ModelLoadState::Loaded;
        return true;
    }

}
