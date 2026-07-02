#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

class Dx12Device {
public:
    Dx12Device() = default;
    ~Dx12Device() = default;

    // 初期化（コマンド周りや記述子サイズ取得も一括で行う）
    bool Initialize();

    // ゲッター
    Microsoft::WRL::ComPtr<ID3D12Device> GetDevice()const { return device_; }
    Microsoft::WRL::ComPtr<IDXGIFactory7> GetDxgiFactory() const { return dxgiFactory_; }
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const { return commandAllocator_; }
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const { return commandQueue_; }
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() const { return commandList_; }

    // 記述子サイズのゲッター
    UINT GetDescriptorSizeSRV() const { return descriptorSizeSRV_; }
    UINT GetDescriptorSizeRTV() const { return descriptorSizeRTV_; }
    UINT GetDescriptorSizeDSV() const { return descriptorSizeDSV_; }

    // main.cpp から移行すると便利なヘルパー関数
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

private:
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
    Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Device>  device_ = nullptr;

    // コマンド関連の追加
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;

    // 記述子サイズのキャッシュ
    UINT descriptorSizeSRV_ = 0;
    UINT descriptorSizeRTV_ = 0;
    UINT descriptorSizeDSV_ = 0;
};