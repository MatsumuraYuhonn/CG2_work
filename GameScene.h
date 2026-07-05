#pragma once
#include <Windows.h>
#include <wrl.h>
#include <memory>
#include <d3d12.h>
#include <xaudio2.h>

#include "Vector.h"
#include "Matrix.h"
#include "Transform.h"
#include "DebugCamera.h"
#include "Model.h"
#include "Sprite.h"
#include "Sound.h"
#include "ConstantBuffer.h"
#include "TextureManager.h"
#include "DescriptorHeapManager.h"

class GameScene {
public:
    GameScene() = default;
    ~GameScene() = default;

    // 初期化・更新・描画
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DescriptorHeapManager* srvHeapManager, IXAudio2* xAudio2);
    void Update(int clientWidth, int clientHeight, const Input* input); 
    void Draw(ID3D12GraphicsCommandList* commandList);
    void Finalize();

private:
    // 内部補助関数（Engineから移行）
    Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

private:
    // 定数バッファ
    MaterialConstantBuffer materialResourceSprite_;
    DirectionalLightConstantBuffer directionalLightResource_;
    TransformationMatrixConstantBuffer wvpResource_;

    // マネージャ経由のゲームリソース
    TextureManager textureManager_;
    std::unique_ptr<Model> model_;
    std::unique_ptr<Sprite> sprite_;
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2_{};

    // オーディオ (データと再生コントロール)
    SoundData soundData1_{};

    // カメラ・シーン制御変数
    DebugCamera debugCamera_;
    Transform transform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
    Transform cameraTransform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,-10.0f} };
    bool isDebugCameraActive_ = false;
    bool useMonsterBall_ = false;
    ModelData modelData_;

};
