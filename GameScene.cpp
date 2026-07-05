#include "GameScene.h"
#include <cassert>
#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

Matrix4x4 GameScene::MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
    Matrix4x4 result = { 0 };
    result.m[0][0] = 2.0f / (right - left);
    result.m[1][1] = 2.0f / (top - bottom);
    result.m[2][2] = 1.0f / (farClip - nearClip);
    result.m[3][0] = -(right + left) / (right - left);
    result.m[3][1] = -(top + bottom) / (top - bottom);
    result.m[3][2] = -nearClip / (farClip - nearClip);
    result.m[3][3] = 1.0f;
    return result;
}

void GameScene::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DescriptorHeapManager* srvHeapManager, IXAudio2* xAudio2) {
    // モデル読み込み
    modelData_ = Model::LoadObjFile("Resources", "axis.obj");
    model_ = std::make_unique<Model>();
    model_->Initialize(device, modelData_);

    // 定数バッファの初期化
    materialResourceSprite_.Initialize(device);
    directionalLightResource_.Initialize(device);
    wvpResource_.Initialize(device);

    // テクスチャマネージャとロード
    textureManager_.Initialize(device, srvHeapManager);
    textureManager_.Load("resources/uvChecker.png", commandList);
    textureManager_.Load(modelData_.material.textureFilePath, commandList);

    textureSrvHandleGPU_ = textureManager_.GetGPUDescriptorHandle("resources/uvChecker.png");
    textureSrvHandleGPU2_ = textureManager_.GetGPUDescriptorHandle(modelData_.material.textureFilePath);

    // スプライト初期化
    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize(device, 640, 360, textureSrvHandleGPU_);

    // オーディオ読み込みと再生
    soundData1_ = SoundLoadWave("Resources/Alarm01.wav");
    SoundPlayWave(xAudio2, &soundData1_);

    // カメラ初期化
    debugCamera_.Initialize();

    // 初期パラメータ設定
    materialResourceSprite_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialResourceSprite_->enabledLighting = 1;
    materialResourceSprite_->uvTransform = MakeIdentityMatrix();

    directionalLightResource_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    directionalLightResource_->direction = Vector3(0.0f, -1.0f, 1.0f);
    directionalLightResource_->intensity = 1.0f;
}

void GameScene::Update(int clientWidth, int clientHeight, const Input* input) {

    // デバッグカメラの切り替えキー判定
    if (input->TriggerKey(DIK_1)) {
        isDebugCameraActive_ = !isDebugCameraActive_;
    }

    Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 viewProjectionMatrix;

    if (isDebugCameraActive_) {
        debugCamera_.Update(input);
        viewProjectionMatrix = Multiply(debugCamera_.GetViewMatrix(), debugCamera_.GetProjectionMatrix());
    }
    else {
        Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform_.scale, cameraTransform_.rotate, cameraTransform_.translate);
        Matrix4x4 viewMatrix = Inverse(cameraMatrix);
        Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(clientWidth) / float(clientHeight), 0.1f, 100.0f);
        viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);
    }

    wvpResource_->WVP = Multiply(worldMatrix, viewProjectionMatrix);
    wvpResource_->World = worldMatrix;

    Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(clientWidth), float(clientHeight), 0.0f, 100.0f);
    sprite_->Update(projectionMatrixSprite);

#ifdef USE_IMGUI
    ImGui::Begin("Debug Settings");
    ImGui::DragFloat3("Sprite Position", &sprite_->transform.translate.x, 1.0f);
    ImGui::DragFloat3("Sprite Rotation", &sprite_->transform.rotate.x, 0.01f);
    ImGui::DragFloat3("Sprite Scale", &sprite_->transform.scale.x, 0.01f);
    ImGui::Checkbox("useMonsterBall", &useMonsterBall_);
    ImGui::ColorEdit4("Light Color", &directionalLightResource_->color.x);

    if (ImGui::DragFloat3("Light Direction", &directionalLightResource_->direction.x, 0.01f, -1.0f, 1.0f)) {
        float length = std::sqrt(directionalLightResource_->direction.x * directionalLightResource_->direction.x +
            directionalLightResource_->direction.y * directionalLightResource_->direction.y +
            directionalLightResource_->direction.z * directionalLightResource_->direction.z);
        if (length != 0) {
            directionalLightResource_->direction.x /= length;
            directionalLightResource_->direction.y /= length;
            directionalLightResource_->direction.z /= length;
        }
    }

    ImGui::DragFloat("Intensity", &directionalLightResource_->intensity, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat3("UVTranslate", &sprite_->uvTransform.translate.x, 0.01f, -10.0f, 10.0f);
    ImGui::DragFloat3("UVScale", &sprite_->uvTransform.scale.x, 0.01f, -10.0f, 10.0f);
    ImGui::SliderAngle("UVRotate", &sprite_->uvTransform.rotate.z, -360.0f, 360.0f);
    ImGui::Text("Model Transform");
    ImGui::DragFloat3("Model Scale", &transform_.scale.x, 0.01f);
    ImGui::DragFloat3("Model Rotate", &transform_.rotate.x, 0.01f);
    ImGui::DragFloat3("Model Translate", &transform_.translate.x, 0.1f);
    ImGui::Checkbox("Debug Camera", &isDebugCameraActive_);
    ImGui::End();
#endif
}

void GameScene::Draw(ID3D12GraphicsCommandList* commandList) {
    // パイプラインに各種定数バッファやデジクリプタテーブルをバインドして描画
    commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite_.GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, wvpResource_.GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(2, directionalLightResource_.GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(3, textureSrvHandleGPU_);

    // モデルの描画
    model_->Draw(commandList);

    // スプライトの描画
    sprite_->Draw(commandList, directionalLightResource_.GetResource());
}

void GameScene::Finalize() {
    SoundUnload(&soundData1_);
}