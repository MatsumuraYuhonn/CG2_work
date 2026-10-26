#include <cassert>
#include <memory>
#include <optional>

#include "MTEngine/Engine/Base/Engine.h"
#include "MTEngine/Engine/Base/Window.h"
#include "MTEngine/Engine/Graphics/ConstantBuffer.h"
#include "MTEngine/Engine/Graphics/Model.h"
#include "MTEngine/Engine/Graphics/Sprite.h"
#include "MTEngine/Engine/Graphics/TextureManager.h"
#include "MTEngine/Engine/Math/Matrix.h"
#include "MTEngine/Engine/Scene/SceneObject.h"
#include "MTEngine/Engine/Scene/SceneSerializer.h"
#include "MTEngine/Engine/Scene/Scene.h"

namespace {

    MTEngine::Matrix4x4 MakeOrthographicMatrix(
        float left,
        float top,
        float right,
        float bottom,
        float nearClip,
        float farClip) {

        MTEngine::Matrix4x4 result{};
        result.m[0][0] = 2.0f / (right - left);
        result.m[1][1] = 2.0f / (top - bottom);
        result.m[2][2] = 1.0f / (farClip - nearClip);
        result.m[3][0] = (left + right) / (left - right);
        result.m[3][1] = (top + bottom) / (bottom - top);
        result.m[3][2] = nearClip / (nearClip - farClip);
        result.m[3][3] = 1.0f;
        return result;
    }

    class GameApplication final : public MTEngine::Engine {
    protected:
        void OnInitialize() override {
            const bool loaded =
                MTEngine::SceneSerializer::Load(
                    kSceneFilePath,
                    GetScene());
            assert(loaded);

            Microsoft::WRL::ComPtr<ID3D12Device> device = GetDevice();
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList =
                GetCommandList();

            textureManager_.Initialize(
                device,
                GetRenderer()->GetSrvHeapManager());

            const bool spriteTextureLoaded = textureManager_.Load(
                kSpriteTexturePath,
                commandList);
            assert(spriteTextureLoaded);
            if (!spriteTextureLoaded) {
                return;
            }

            D3D12_GPU_DESCRIPTOR_HANDLE spriteTextureHandle{};
            const bool foundSpriteTexture =
                textureManager_.TryGetGPUDescriptorHandle(
                    kSpriteTexturePath,
                    spriteTextureHandle);
            assert(foundSpriteTexture);
            if (!foundSpriteTexture) {
                return;
            }

            sprite_.Initialize(
                device,
                128,
                128,
                spriteTextureHandle);
            sprite_.transform.translate = { 32.0f, 32.0f, 0.0f };

            const std::optional<MTEngine::ModelData> modelData =
                MTEngine::Model::LoadObjFile(
                    kModelDirectory,
                    kModelFilename);
            assert(modelData.has_value());
            if (!modelData.has_value()) {
                return;
            }

            for (const MTEngine::MeshData& mesh : modelData->meshes) {
                const std::string& texturePath =
                    mesh.material.textureFilePath;
                if (texturePath.empty()) {
                    continue;
                }

                const bool textureLoaded =
                    textureManager_.Load(texturePath, commandList);
                assert(textureLoaded);
                if (!textureLoaded) {
                    return;
                }
            }

            const bool modelInitialized =
                model_.Initialize(device, modelData.value());
            assert(modelInitialized);
            if (!modelInitialized) {
                return;
            }

            const bool transformInitialized =
                modelTransformBuffer_.Initialize(device);
            assert(transformInitialized);
            if (!transformInitialized) {
                return;
            }

            const bool lightInitialized = lightBuffer_.Initialize(device);
            assert(lightInitialized);
            if (!lightInitialized) {
                return;
            }

            lightBuffer_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
            lightBuffer_->direction = { 0.0f, -1.0f, 1.0f };
            lightBuffer_->intensity = 1.0f;

            modelTransform_.translate = { 0.0f, 0.0f, 0.0f };
            graphicsInitialized_ = true;
        }

        void OnUpdate() override {
            if (!graphicsInitialized_) {
                return;
            }

            modelTransform_.rotate.y += 0.01f;

            const MTEngine::Matrix4x4 world =
                MTEngine::MakeAffineMatrix(
                    modelTransform_.scale,
                    modelTransform_.rotate,
                    modelTransform_.translate);
            const MTEngine::Matrix4x4 worldView =
                MTEngine::Multiply(
                    world,
                    GetDebugCamera().GetViewMatrix());

            modelTransformBuffer_->World = world;
            modelTransformBuffer_->WVP =
                MTEngine::Multiply(
                    worldView,
                    GetDebugCamera().GetProjectionMatrix());

            const MTEngine::Matrix4x4 spriteProjection =
                MakeOrthographicMatrix(
                    0.0f,
                    0.0f,
                    static_cast<float>(MTEngine::Window::kClientWidth),
                    static_cast<float>(MTEngine::Window::kClientHeight),
                    0.0f,
                    100.0f);
            sprite_.Update(spriteProjection);
        }

        void OnDraw() override {
            if (!graphicsInitialized_) {
                return;
            }

            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList =
                GetCommandList();

            commandList->SetGraphicsRootConstantBufferView(
                1,
                modelTransformBuffer_.GetGPUVirtualAddress());
            commandList->SetGraphicsRootConstantBufferView(
                2,
                lightBuffer_.GetGPUVirtualAddress());
            model_.DrawTextured(commandList, &textureManager_);

            GetRenderer()->ClearSceneDepth();
            sprite_.Draw(commandList, lightBuffer_.GetResource());
        }

        void OnFinalize() override {
            MTEngine::SceneSerializer::Save(
                kSceneFilePath,
                GetScene());
            graphicsInitialized_ = false;
        }

    private:
        static constexpr const char* kSceneFilePath =
            "MTEngine/Assets/Scenes/main.scene.json";
        static constexpr const char* kSpriteTexturePath =
            "MTEngine/Assets/Resources/monsterBall.png";
        static constexpr const char* kModelDirectory =
            "MTEngine/Assets/Resources";
        static constexpr const char* kModelFilename = "teapot.obj";

        MTEngine::TextureManager textureManager_;
        MTEngine::DirectionalLightConstantBuffer lightBuffer_;
        MTEngine::TransformationMatrixConstantBuffer
            modelTransformBuffer_;
        MTEngine::Sprite sprite_;
        MTEngine::Model model_;
        MTEngine::Transform modelTransform_;
        bool graphicsInitialized_ = false;
    };
}

int WINAPI WinMain(
    _In_ HINSTANCE,
    _In_opt_ HINSTANCE,
    _In_ LPSTR,
    _In_ int) {

    auto game = std::make_unique<GameApplication>();

    game->Initialize();
    game->Run();
    game->Finalize();

    return 0;
}
