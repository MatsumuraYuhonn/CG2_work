#include "GraphicsManager.h"

namespace MTEngine {

    void GraphicsManager::Initialize(HWND hwnd, int32_t width, int32_t height) {
#ifdef _DEBUG
        Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
            debugController->SetEnableGPUBasedValidation(TRUE);
        }
#endif

        // DirectX12デバイスの初期化
        bool isInitialized = dx12Device_.Initialize();
        assert(isInitialized);

        auto device = dx12Device_.GetDevice();
        auto commandQueue = dx12Device_.GetCommandQueue();
        auto dxgiFactory = dx12Device_.GetDxgiFactory();

        // DescriptorHeapManagerの初期化
        rtvHeapManager_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
        dsvHeapManager_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
        srvHeapManager_.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

#ifdef _DEBUG
        Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
        if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
            D3D12_MESSAGE_ID denyIds[] = { D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE };
            D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
            D3D12_INFO_QUEUE_FILTER filter{};
            filter.DenyList.NumIDs = _countof(denyIds);
            filter.DenyList.pIDList = denyIds;
            filter.DenyList.NumSeverities = _countof(severities);
            filter.DenyList.pSeverityList = severities;
            infoQueue->PushStorageFilter(&filter);
        }
#endif

        // スワップチェーンの作成
        bool isSwapChainInit = swapChain_.Initialize(dxgiFactory, commandQueue, hwnd, width, height);
        assert(isSwapChainInit);

        // RTVおよびDSVの作成
        CreateRenderTargetViews();
        CreateDepthStencilView(width, height);

        // ビューポートとシザー矩形の設定
        viewport_.Width = static_cast<float>(width);
        viewport_.Height = static_cast<float>(height);
        viewport_.TopLeftX = 0.0f;
        viewport_.TopLeftY = 0.0f;
        viewport_.MinDepth = 0.0f;
        viewport_.MaxDepth = 1.0f;

        scissorRect_.left = 0;
        scissorRect_.right = width;
        scissorRect_.top = 0;
        scissorRect_.bottom = height;
    }

    void GraphicsManager::CreateRenderTargetViews() {
        auto device = dx12Device_.GetDevice();
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        for (uint32_t i = 0; i < 2; ++i) {
            rtvHandles_[i] = rtvHeapManager_.GetCPUDescriptorHandle(i);
            device->CreateRenderTargetView(swapChain_.GetBuffer(i).Get(), &rtvDesc, rtvHandles_[i]);
        }
    }

    void GraphicsManager::CreateDepthStencilView(int32_t width, int32_t height) {
        auto device = dx12Device_.GetDevice();

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

        HRESULT hr = device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthClearValue,
            IID_PPV_ARGS(&depthStencilResource_)
        );
        assert(SUCCEEDED(hr));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvHeapManager_.GetCPUDescriptorHandle(0));
    }

    void GraphicsManager::BeginDraw() {
        auto commandList = dx12Device_.GetCommandList();
        UINT backBufferIndex = swapChain_.GetCurrentBackBufferIndex();

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = swapChain_.GetBuffer(backBufferIndex).Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        commandList->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeapManager_.GetCPUDescriptorHandle(0);
        commandList->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, &dsvHandle);

        float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        commandList->RSSetViewports(1, &viewport_);
        commandList->RSSetScissorRects(1, &scissorRect_);

        ID3D12DescriptorHeap* descriptorHeaps[] = { srvHeapManager_.GetHeap() };
        commandList->SetDescriptorHeaps(1, descriptorHeaps);
    }

    void GraphicsManager::EndDraw() {
        auto commandList = dx12Device_.GetCommandList();
        auto commandAllocator = dx12Device_.GetCommandAllocator();
        auto commandQueue = dx12Device_.GetCommandQueue();
        UINT backBufferIndex = swapChain_.GetCurrentBackBufferIndex();

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = swapChain_.GetBuffer(backBufferIndex).Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

        commandList->ResourceBarrier(1, &barrier);

        HRESULT hr = commandList->Close();
        assert(SUCCEEDED(hr));

        ID3D12CommandList* commandLists[] = { commandList.Get() };
        commandQueue->ExecuteCommandLists(1, commandLists);

        swapChain_.Present(1, 0);
        dx12Device_.WaitForGPU();

        hr = commandAllocator->Reset();
        assert(SUCCEEDED(hr));

        hr = commandList->Reset(commandAllocator.Get(), nullptr);
        assert(SUCCEEDED(hr));
    }

}