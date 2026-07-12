#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

// デスクリプタヒープの生成およびハンドル取得を管理するクラス
class DescriptorHeapManager {
public:
    DescriptorHeapManager() = default;
    ~DescriptorHeapManager() = default;

    // 初期化：ヒープの生成と各種サイズの取得
    void Initialize(
        Microsoft::WRL::ComPtr<ID3D12Device> device,
        D3D12_DESCRIPTOR_HEAP_TYPE heapType,
        UINT numDescriptors,
        bool shaderVisible
    );

    // ヒープ本体を取得
    ID3D12DescriptorHeap* GetHeap() const { return descriptorHeap_.Get(); }
    // デスクリプタのサイズを取得
    UINT GetDescriptorSize() const { return descriptorSize_; }

    // インデックスに応じたCPU/GPUハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index) const;

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_ = nullptr;
    UINT descriptorSize_ = 0;
};