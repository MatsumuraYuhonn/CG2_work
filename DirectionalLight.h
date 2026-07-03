#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Vector.h" // Vector3, Vector4が定義されている想定

class DirectionalLight {
public:
    // シェーダーの構造体と一致させるデータ構造
    struct ConstBufferData {
        Vector4 color;
        Vector3 direction;
        float intensity;
    };

    DirectionalLight() = default;
    ~DirectionalLight() = default;

    // 初期化: 定数バッファの生成と初期値の設定
    void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device);

    // データの更新をバッファに転送
    void Update();

    // 描画時にコマンドリストへバインド
    void Bind(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT rootParameterIndex);

    // ゲッター群
    ID3D12Resource* GetResource() const { return constBuffer_.Get(); }
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return constBuffer_->GetGPUVirtualAddress(); }

public:
    // 外部から数値を直接変更できるようにパブリック変数として露出（ImGui用）
    ConstBufferData data;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
    ConstBufferData* mappedData_ = nullptr;
};
