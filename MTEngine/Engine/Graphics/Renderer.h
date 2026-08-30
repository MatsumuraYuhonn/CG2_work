#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cassert>
#include <memory>
#include "MTEngine/Engine/Base/Dx12Device.h"
#include "MTEngine/Engine/Base/SwapChain.h"
#include "MTEngine/Engine/Graphics/DescriptorHeapManager.h"
#include "MTEngine/Engine/Graphics/PipelineManager.h"

namespace MTEngine {

    class Renderer {
    public:
        Renderer() = default;
        ~Renderer() = default;

        void Initialize(Dx12Device* dx12Device, SwapChain* swapChain, PipelineManager* pipelineManager, int32_t width, int32_t height);

        // 描画フレームの開始処理（クリア・バリア遷移・ビューポート設定など）
        void BeginFrame();

        void BeginScene();
        void EndScene();
        void ClearSceneDepth();
        void CopySceneToBackBuffer();

        // 描画フレームの終了処理（Present・GPU同期・コマンドリストリセット）
        void EndFrame();

        // 各種ゲッター
        ID3D12GraphicsCommandList* GetCommandList() const { return dx12Device_->GetCommandList().Get(); }
        DescriptorHeapManager* GetSrvHeapManager() { return &srvHeapManager_; }
        D3D12_GPU_DESCRIPTOR_HANDLE GetSceneTextureHandle() const { return sceneTextureSrvHandleGPU_; }

    private:
        void CreateDepthStencilTexture(int32_t width, int32_t height);
        void CreateSceneRenderTarget(int32_t width, int32_t height);

        Dx12Device* dx12Device_ = nullptr;
        SwapChain* swapChain_ = nullptr;
        PipelineManager* pipelineManager_ = nullptr;

        DescriptorHeapManager rtvHeapManager_;
        DescriptorHeapManager dsvHeapManager_;
        DescriptorHeapManager srvHeapManager_;

        Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
        Microsoft::WRL::ComPtr<ID3D12Resource> sceneDepthStencilResource_;
        Microsoft::WRL::ComPtr<ID3D12Resource> sceneRenderTargetResource_;
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2] = {};
        D3D12_CPU_DESCRIPTOR_HANDLE sceneRtvHandle_{};
        D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureSrvHandleGPU_{};
        D3D12_VIEWPORT viewport_{};
        D3D12_RECT scissorRect_{};
    };

}
