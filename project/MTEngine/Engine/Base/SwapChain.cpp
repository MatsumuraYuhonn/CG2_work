#include "SwapChain.h"
#include <cassert>


namespace MTEngine {

    bool SwapChain::Initialize(
        Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory,
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
        HWND hwnd,
        int32_t width,
        int32_t height)
    {
        HRESULT hr = S_OK;

        // スワップチェーンの設定
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = kBufferCount; // 定数を使用
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        // スワップチェーンの生成
        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
        hr = dxgiFactory->CreateSwapChainForHwnd(
            commandQueue.Get(),
            hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1
        );
        assert(SUCCEEDED(hr));

        hr = swapChain1.As(&swapChain_);
        assert(SUCCEEDED(hr));


        for (UINT i = 0; i < kBufferCount; ++i) {
            hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
            assert(SUCCEEDED(hr));
        }

        return true;
    }


}