#include "Renderer.h"

namespace MTEngine {

    void Renderer::Initialize(Dx12Device* dx12Device, SwapChain* swapChain, PipelineManager* pipelineManager, int32_t width, int32_t height) {
        dx12Device_ = dx12Device;
        swapChain_ = swapChain;
        pipelineManager_ = pipelineManager;

        auto device = dx12Device_->GetDevice();

        // ヒープの初期化
        rtvHeapManager_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3, false);
        dsvHeapManager_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2, false);
        srvHeapManager_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

        // RTV作成
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        rtvHandles_[0] = rtvHeapManager_.GetCPUDescriptorHandle(0);
        device->CreateRenderTargetView(swapChain_->GetBuffer(0).Get(), &rtvDesc, rtvHandles_[0]);

        rtvHandles_[1] = rtvHeapManager_.GetCPUDescriptorHandle(1);
        device->CreateRenderTargetView(swapChain_->GetBuffer(1).Get(), &rtvDesc, rtvHandles_[1]);

        // DSV作成
        CreateDepthStencilTexture(width, height);

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvHeapManager_.GetCPUDescriptorHandle(0));

        CreateSceneRenderTarget(width, height);
        sceneRtvHandle_ = rtvHeapManager_.GetCPUDescriptorHandle(2);
        device->CreateRenderTargetView(sceneRenderTargetResource_.Get(), &rtvDesc, sceneRtvHandle_);
        device->CreateDepthStencilView(sceneDepthStencilResource_.Get(), &dsvDesc, dsvHeapManager_.GetCPUDescriptorHandle(1));

        D3D12_SHADER_RESOURCE_VIEW_DESC sceneSrvDesc{};
        sceneSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        sceneSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        sceneSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        sceneSrvDesc.Texture2D.MipLevels = 1;
        // 0番はImGuiのフォント、1番はSceneビュー用に予約する。
        sceneTextureSrvHandleGPU_ = srvHeapManager_.GetGPUDescriptorHandle(1);
        device->CreateShaderResourceView(sceneRenderTargetResource_.Get(), &sceneSrvDesc, srvHeapManager_.GetCPUDescriptorHandle(1));

        // ビューポート・シザー矩形
        viewport_.Width = static_cast<float>(width);
        viewport_.Height = static_cast<float>(height);
        viewport_.TopLeftX = 0;
        viewport_.TopLeftY = 0;
        viewport_.MinDepth = 0.0f;
        viewport_.MaxDepth = 1.0f;

        scissorRect_.left = 0;
        scissorRect_.right = width;
        scissorRect_.top = 0;
        scissorRect_.bottom = height;
    }

    void Renderer::CreateDepthStencilTexture(int32_t width, int32_t height) {
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Width = width;
        resourceDesc.Height = height;
        resourceDesc.MipLevels = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_CLEAR_VALUE depthClearValue{};
        depthClearValue.DepthStencil.Depth = 1.0f;
        depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

        HRESULT hr = dx12Device_->GetDevice()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthClearValue,
            IID_PPV_ARGS(&depthStencilResource_)
        );
        assert(SUCCEEDED(hr));
    }

    void Renderer::CreateSceneRenderTarget(int32_t width, int32_t height) {
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC colorDesc{};
        colorDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        colorDesc.Width = width;
        colorDesc.Height = height;
        colorDesc.DepthOrArraySize = 1;
        colorDesc.MipLevels = 1;
        colorDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        colorDesc.SampleDesc.Count = 1;
        colorDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE colorClearValue{};
        colorClearValue.Format = colorDesc.Format;
        colorClearValue.Color[0] = 0.1f;
        colorClearValue.Color[1] = 0.25f;
        colorClearValue.Color[2] = 0.5f;
        colorClearValue.Color[3] = 1.0f;

        HRESULT hr = dx12Device_->GetDevice()->CreateCommittedResource(
            &heapProperties, D3D12_HEAP_FLAG_NONE, &colorDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &colorClearValue,
            IID_PPV_ARGS(&sceneRenderTargetResource_));
        assert(SUCCEEDED(hr));

        D3D12_RESOURCE_DESC depthDesc{};
        depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthDesc.Width = width;
        depthDesc.Height = height;
        depthDesc.DepthOrArraySize = 1;
        depthDesc.MipLevels = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthClearValue{};
        depthClearValue.Format = depthDesc.Format;
        depthClearValue.DepthStencil.Depth = 1.0f;

        hr = dx12Device_->GetDevice()->CreateCommittedResource(
            &heapProperties, D3D12_HEAP_FLAG_NONE, &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue,
            IID_PPV_ARGS(&sceneDepthStencilResource_));
        assert(SUCCEEDED(hr));
    }

    void Renderer::BeginFrame() {
        auto commandList = dx12Device_->GetCommandList();
        UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

        // リソースバリア (PRESENT -> RENDER_TARGET)
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = swapChain_->GetBuffer(backBufferIndex).Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        commandList->ResourceBarrier(1, &barrier);

        // レンダーターゲット設定＆クリア
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeapManager_.GetCPUDescriptorHandle(0);
        commandList->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, &dsvHandle);

        float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        commandList->RSSetViewports(1, &viewport_);
        commandList->RSSetScissorRects(1, &scissorRect_);

        pipelineManager_->Bind(commandList);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ID3D12DescriptorHeap* descriptorHeaps[] = { srvHeapManager_.GetHeap() };
        commandList->SetDescriptorHeaps(1, descriptorHeaps);
    }

    void Renderer::BeginScene() {
        auto commandList = dx12Device_->GetCommandList();

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = sceneRenderTargetResource_.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE sceneDsvHandle = dsvHeapManager_.GetCPUDescriptorHandle(1);
        commandList->OMSetRenderTargets(1, &sceneRtvHandle_, false, &sceneDsvHandle);

        float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
        commandList->ClearRenderTargetView(sceneRtvHandle_, clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(sceneDsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        commandList->RSSetViewports(1, &viewport_);
        commandList->RSSetScissorRects(1, &scissorRect_);
        pipelineManager_->Bind(commandList);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void Renderer::EndScene() {
        auto commandList = dx12Device_->GetCommandList();
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = sceneRenderTargetResource_.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);

        const UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();
        D3D12_CPU_DESCRIPTOR_HANDLE backBufferDsvHandle = dsvHeapManager_.GetCPUDescriptorHandle(0);
        commandList->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, &backBufferDsvHandle);
    }

    void Renderer::ClearSceneDepth() {
        auto commandList = dx12Device_->GetCommandList();
        const D3D12_CPU_DESCRIPTOR_HANDLE sceneDsvHandle =
            dsvHeapManager_.GetCPUDescriptorHandle(1);
        commandList->ClearDepthStencilView(
            sceneDsvHandle,
            D3D12_CLEAR_FLAG_DEPTH,
            1.0f,
            0,
            0,
            nullptr);
    }

    void Renderer::CopySceneToBackBuffer() {
        auto commandList = dx12Device_->GetCommandList();
        const UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();
        ID3D12Resource* backBuffer = swapChain_->GetBuffer(backBufferIndex).Get();

        D3D12_RESOURCE_BARRIER barriers[2]{};
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Transition.pResource = sceneRenderTargetResource_.Get();
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Transition.pResource = backBuffer;
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(2, barriers);

        commandList->CopyResource(backBuffer, sceneRenderTargetResource_.Get());

        std::swap(barriers[0].Transition.StateBefore, barriers[0].Transition.StateAfter);
        std::swap(barriers[1].Transition.StateBefore, barriers[1].Transition.StateAfter);
        commandList->ResourceBarrier(2, barriers);
    }

    void Renderer::EndFrame() {
        auto commandList = dx12Device_->GetCommandList();
        auto commandAllocator = dx12Device_->GetCommandAllocator();
        auto commandQueue = dx12Device_->GetCommandQueue();
        UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

        // リソースバリア (RENDER_TARGET -> PRESENT)
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = swapChain_->GetBuffer(backBufferIndex).Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        commandList->ResourceBarrier(1, &barrier);

        HRESULT hr = commandList->Close();
        assert(SUCCEEDED(hr));

        ID3D12CommandList* commandLists[] = { commandList.Get() };
        commandQueue->ExecuteCommandLists(1, commandLists);

        swapChain_->Present(1, 0);
        dx12Device_->WaitForGPU();

        hr = commandAllocator->Reset();
        assert(SUCCEEDED(hr));

        hr = commandList->Reset(commandAllocator.Get(), nullptr);
        assert(SUCCEEDED(hr));
    }

}
