#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstdint>
#include <cassert>

// スワップチェーン（フロント・バックバッファ）の管理クラス
class SwapChain {
public:
    SwapChain() = default;
    ~SwapChain() = default;

    // 初期化: 必要になる各種デバイスやファクトリ、ウィンドウハンドルを渡します
    bool Initialize(
        Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory,
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
        HWND hwnd,
        int32_t width,
        int32_t height
    );

    UINT GetBufferCount() const { return kBufferCount; }

    // バックバッファのインデックスを取得
    UINT GetCurrentBackBufferIndex() const { return swapChain_->GetCurrentBackBufferIndex(); }

    // Present (画面フリップ)
    void Present(UINT syncInterval, UINT flags) { swapChain_->Present(syncInterval, flags); }

    // 各バッファのリソースを取得
    Microsoft::WRL::ComPtr<ID3D12Resource> GetBuffer(UINT index) const { return swapChainResources_[index]; }

    // スワップチェーン本体の取得
    Microsoft::WRL::ComPtr<IDXGISwapChain4> Get() const { return swapChain_; }

private:
    static constexpr UINT kBufferCount = 2;

    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2] = { nullptr };
};