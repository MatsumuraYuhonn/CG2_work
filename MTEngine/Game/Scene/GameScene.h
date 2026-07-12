#pragma once
#include <Windows.h>
#include <wrl.h>
#include <memory>
#include <d3d12.h>
#include <xaudio2.h>

#include "MTEngine/Engine/Math/Vector.h"
#include "MTEngine/Engine/Math/Matrix.h"
#include "MTEngine/Engine/Math/Transform.h"
#include "MTEngine/Engine/Debug/DebugCamera.h"
#include "MTEngine/Engine/Graphics/Model.h"
#include "MTEngine/Engine/Graphics/Sprite.h"
#include "MTEngine/Engine/Audio/Sound.h"
#include "MTEngine/Engine/Graphics/ConstantBuffer.h"
#include "MTEngine/Engine/Graphics/TextureManager.h"
#include "MTEngine/Engine/Graphics/DescriptorHeapManager.h"

//ゲームシーンを管理するクラス
class GameScene {
public:
    //コンストラクタ
    GameScene() = default;
    //デストラクタ
    ~GameScene() = default;

    //初期化
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DescriptorHeapManager* srvHeapManager, IXAudio2* xAudio2);
    //更新処理
    void Update(int clientWidth, int clientHeight, const Input* input);
    //描画処理
    void Draw(ID3D12GraphicsCommandList* commandList);
    //終了処理
    void Finalize();

private:
    //平行投影行列の作成
    Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

private:
    //スプライト用のマテリアル定数バッファ
    MaterialConstantBuffer materialResourceSprite_;
    //平行光源用の定数バッファ
    DirectionalLightConstantBuffer directionalLightResource_;
    //行列変換用の定数バッファ
    TransformationMatrixConstantBuffer wvpResource_;

    //テクスチャマネージャ
    TextureManager textureManager_;
    //モデルオブジェクト
    std::unique_ptr<Model> model_;
    //スプライトオブジェクト
    std::unique_ptr<Sprite> sprite_;
    //テクスチャのGPUデスクリプタハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};
    //テクスチャ2のGPUデスクリプタハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2_{};

    //オーディオデータ
    SoundData soundData1_{};

    //デバッグカメラ
    DebugCamera debugCamera_;
    //オブジェクトのトランスフォーム情報
    Transform transform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
    //カメラのトランスフォーム情報
    Transform cameraTransform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,-10.0f} };
    //デバッグカメラが有効かどうか
    bool isDebugCameraActive_ = false;
    //モンスターボールを使用するかどうか
    bool useMonsterBall_ = false;
    //モデルの形状データ
    ModelData modelData_;

};