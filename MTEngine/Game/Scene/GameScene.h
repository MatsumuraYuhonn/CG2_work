#pragma once
#include <Windows.h>
#include <wrl.h>
#include <memory>
#include <vector>
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
namespace MTEngine {

    enum class DebugMode {
        Sprite_Axis_Sphere,
        MultiMesh,
        MultiMaterial,
        Sound
    };

    class GameScene {
    public:
        GameScene() = default;
        ~GameScene() = default;

        void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DescriptorHeapManager* srvHeapManager, IXAudio2* xAudio2);
        void Update(int clientWidth, int clientHeight, const Input* input, IXAudio2* xAudio2);
        void Draw(ID3D12GraphicsCommandList* commandList);
        void Finalize();

    private:
        //平行投影行列の作成
        Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);
        //球体の頂点データ生成
        std::vector<VertexData> MakeSphere(uint32_t subdivision);

    private:
        //スプライト用のマテリアル定数バッファ
        MaterialConstantBuffer materialResourceSprite_;
        //平行光源用の定数バッファ
        DirectionalLightConstantBuffer directionalLightResource_;
        //行列変換用の定数バッファ（axis.objモデル用）
        TransformationMatrixConstantBuffer wvpResource_;
        //行列変換用の定数バッファ（球体用）
        TransformationMatrixConstantBuffer sphereWvpResource_;

        //テクスチャマネージャ
        TextureManager textureManager_;

        //axis.objモデルオブジェクト
        std::unique_ptr<Model> model_;
        //axis.objのモデルデータ
        ModelData modelData_;

        //球体モデルオブジェクト
        std::unique_ptr<Model> sphereModel_;
        //球体のモデルデータ
        ModelData sphereModelData_;

        //スプライトオブジェクト
        std::unique_ptr<Sprite> sprite_;


        // 複数メッシュのモデル
        std::unique_ptr<Model> multiMeshModel_;
        //multiMesh.objのモデルデータ
        ModelData multiMeshModelData_;
        //multiMesh用の行列変換定数バッファ
        TransformationMatrixConstantBuffer multiMeshWvpResource_;
        //multiMesh用テクスチャのGPUデスクリプタハンドル
        D3D12_GPU_DESCRIPTOR_HANDLE multiMeshTextureSrvHandleGPU_{};
        //multiMeshのトランスフォーム情報
        Transform multiMeshTransform_{ {1.0f,1.0f,1.0f}, {0.0f,3.0f,0.0f}, {0.0f,-1.0f,5.0f} };


        // マルチマテリアルモデル（1つのモデルに複数のマテリアル/テクスチャを持つ）
        std::unique_ptr<Model> multiMaterialModel_;
        //マルチマテリアルモデルのモデルデータ
        ModelData multiMaterialModelData_;
        //マルチマテリアル用の行列変換定数バッファ
        TransformationMatrixConstantBuffer multiMaterialWvpResource_;
        //マルチマテリアルモデルのトランスフォーム情報
        Transform multiMaterialTransform_{ {1.0f,1.0f,1.0f}, {0.0f,3.0f,0.0f}, {0.0f,0.0f,0.0f} };

        //axis.obj用テクスチャのGPUデスクリプタハンドル
        D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};
        //球体(uvChecker)用テクスチャのGPUデスクリプタハンドル
        D3D12_GPU_DESCRIPTOR_HANDLE sphereTextureSrvHandleGPU_{};

        //オーディオデータ
        SoundData soundData1_{};

        //デバッグカメラ
        DebugCamera debugCamera_;
        //axis.objのトランスフォーム情報
        Transform transform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,-1.0f,0.0f} };
        //球体のトランスフォーム情報
        Transform sphereTransform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {2.0f,0.0f,0.0f} }; // 位置ずらして両方見えるように
        //カメラのトランスフォーム情報
        Transform cameraTransform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,-10.0f} };
        //デバッグカメラが有効かどうか
        bool isDebugCameraActive_ = false;
        //モンスターボールを使用するかどうか
        bool useMonsterBall_ = false;


        DebugMode currentDrawMode_ = DebugMode::Sprite_Axis_Sphere;

        bool isSoundPlay = false;

    };

}