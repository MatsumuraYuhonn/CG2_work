#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <cassert>

#include "MTEngine/Engine/Base/Dx12Device.h"
#include "MTEngine/Engine/Base/SwapChain.h"
#include "MTEngine/Engine/Graphics/DescriptorHeapManager.h"

namespace MTEngine {

    class GraphicsManager {
    public:
        GraphicsManager() = default;
        ~GraphicsManager() = default;

        void Initialize(HWND hwnd, int32_t width, int32_t height);
        void BeginDraw();
        void EndDraw();

        // ゲッター関数
        Dx12Device* GetDx12Device() { return &dx12Device_; }
        ID3D12Device* GetDevice() const { return dx12Device_.GetDevice().Get(); }
        ID3D12GraphicsCommandList* GetCommandList() const { return dx12Device_.GetCommandList().Get(); }
        DescriptorHeapManager* GetSrvHeapManager() { return &srvHeapManager_; }
        uint32_t GetBufferCount() const { return swapChain_.GetBufferCount(); }

    private:
        void CreateRenderTargetViews();
        void CreateDepthStencilView(int32_t width, int32_t height);

    private:
        Dx12Device dx12Device_;
        SwapChain swapChain_;

        DescriptorHeapManager rtvHeapManager_;
        DescriptorHeapManager dsvHeapManager_;
        DescriptorHeapManager srvHeapManager_;

        Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2] = {};

        D3D12_VIEWPORT viewport_{};
        D3D12_RECT scissorRect_{};
    };

}