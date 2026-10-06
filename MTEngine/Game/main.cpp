#include <cassert>
#include <cstdlib>
#include <memory>

#include "MTEngine/Engine/Base/Engine.h"
#include "MTEngine/Engine/Graphics/ConstantBuffer.h"
#include "MTEngine/Engine/Graphics/Model.h"
#include "MTEngine/Engine/Graphics/TextureManager.h"
#include "MTEngine/Engine/Math/Matrix.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

namespace {

    class GameApplication final : public MTEngine::Engine {
    protected:
        void OnInitialize() override {
            // Engineが所有しているDirectX 12オブジェクトを取得
            Microsoft::WRL::ComPtr<ID3D12Device> device = GetDevice();
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList =
                GetCommandList();

            // plane.objを読み込んで頂点バッファを作成
            MTEngine::ModelData planeData =
                MTEngine::Model::LoadObjFile(
                    "MTEngine/Assets/Resources",
                    "plane.obj");

            const bool isPlaneInitialized =
                planeModel_.Initialize(device, planeData);
            assert(isPlaneInitialized);
            if (!isPlaneInitialized) {
                std::abort();
            }

            // テクスチャ管理を初期化
            textureManager_.Initialize(
                device,
                GetRenderer()->GetSrvHeapManager());

            // MTLに設定されているテクスチャを読み込む
            for (const MTEngine::MeshData& mesh : planeData.meshes) {
                if (!mesh.material.textureFilePath.empty()) {
                    textureManager_.Load(
                        mesh.material.textureFilePath,
                        commandList);
                }
            }

            // シェーダーへ渡す定数バッファを作成
            const bool isMaterialBufferInitialized =
                materialBuffer_.Initialize(device);
            const bool isTransformBufferInitialized =
                transformBuffer_.Initialize(device);
            const bool isLightBufferInitialized =
                lightBuffer_.Initialize(device);

            assert(isMaterialBufferInitialized);
            assert(isTransformBufferInitialized);
            assert(isLightBufferInitialized);
            if (!isMaterialBufferInitialized ||
                !isTransformBufferInitialized ||
                !isLightBufferInitialized) {
                std::abort();
            }

            // UV Checkerに色と平行光源の強さを反映するマテリアル
            MTEngine::Material* material = materialBuffer_.operator->();
            *material = {};
            material->color = materialColor_;
            material->enabledLighting = 1;
            material->uvTransform = MTEngine::MakeIdentityMatrix();
            material->lightingMode = 1;
            material->useTexture = 1;
            material->isSelected = 0;

            // Planeの正面を照らす平行光源
            MTEngine::DirectionalLight* light = lightBuffer_.operator->();
            *light = {};
            light->color = { 1.0f, 1.0f, 1.0f, 1.0f };
            light->direction = { 0.0f, 0.0f, 1.0f };
            light->intensity = lightIntensity_;

            // Planeの大きさと位置
            planeWorldMatrix_ = MTEngine::MakeAffineMatrix(
                { 2.0f, 2.0f, 1.0f }, // 拡大率
                // カメラは-Z側にいるため、Planeの表面をカメラ側へ向ける
                { 0.0f, 3.14159265358979323846f, 0.0f }, // 回転
                { 0.0f, 0.0f, 0.0f }  // 位置
            );

            // Planeが見やすい位置にカメラを置く
            GetDebugCamera().SetTarget({ 0.0f, 0.0f, 0.0f });
            GetDebugCamera().SetDistance(8.0f);

            // PlaneをHierarchyへ追加し、選択時のInspectorを登録する。
            GetEditor()->RegisterGameObject(
                "Plane",
                [this]() { DrawPlaneInspector(); });
        }

        void OnUpdate() override {
            // Inspectorで変更した値をそのフレームでGPU側へ反映する。
            materialBuffer_->color = materialColor_;
            lightBuffer_->intensity = lightIntensity_;
        }

        void OnDraw() override {
            ID3D12GraphicsCommandList* commandList = GetCommandList();

            // Planeに設定されたブレンドモードのPSOへ切り替える。
            GetRenderer()->SetBlendMode(blendMode_);

            // World × View × Projection
            const MTEngine::Matrix4x4 viewProjection =
                MTEngine::Multiply(
                    GetDebugCamera().GetViewMatrix(),
                    GetDebugCamera().GetProjectionMatrix());

            transformBuffer_->World = planeWorldMatrix_;
            transformBuffer_->WVP =
                MTEngine::Multiply(
                    planeWorldMatrix_,
                    viewProjection);

            // b0: マテリアル
            commandList->SetGraphicsRootConstantBufferView(
                0,
                materialBuffer_.GetGPUVirtualAddress());

            // b1: World・WVP行列
            commandList->SetGraphicsRootConstantBufferView(
                1,
                transformBuffer_.GetGPUVirtualAddress());

            // b2: 平行光源
            commandList->SetGraphicsRootConstantBufferView(
                2,
                lightBuffer_.GetGPUVirtualAddress());

            // 頂点バッファとuvChecker.pngを設定して描画
            planeModel_.Draw(commandList, &textureManager_);
        }

        void OnFinalize() override {
            // ComPtrや各クラスが自動的にリソースを解放する
        }

    private:
        void DrawPlaneInspector() {
#ifdef USE_IMGUI
            ImGui::SliderFloat(
                "Light Intensity",
                &lightIntensity_,
                0.0f,
                5.0f,
                "%.2f");
            ImGui::ColorEdit4("Material Color", &materialColor_.x);
            ImGui::TextDisabled("Material Color A controls transparency.");

            constexpr const char* blendModeNames[] = {
                "None",
                "Normal",
                "Add",
                "Subtract",
                "Multiply",
                "Screen"
            };
            int selectedBlendMode = static_cast<int>(blendMode_);
            if (ImGui::Combo(
                "Blend Mode",
                &selectedBlendMode,
                blendModeNames,
                IM_ARRAYSIZE(blendModeNames))) {
                blendMode_ =
                    static_cast<MTEngine::BlendMode>(selectedBlendMode);
            }
#endif
        }

        MTEngine::Model planeModel_;
        MTEngine::TextureManager textureManager_;

        MTEngine::MaterialConstantBuffer materialBuffer_;
        MTEngine::TransformationMatrixConstantBuffer transformBuffer_;
        MTEngine::DirectionalLightConstantBuffer lightBuffer_;

        MTEngine::Vector4 materialColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
        float lightIntensity_ = 1.0f;
        MTEngine::BlendMode blendMode_ =
            MTEngine::BlendMode::kBlendModeNormal;

        MTEngine::Matrix4x4 planeWorldMatrix_ =
            MTEngine::MakeIdentityMatrix();
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
