#include "Engine.h"
#include <cassert>
#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif

// グローバル関数の実装
Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes) {

    D3D12_HEAP_PROPERTIES uploadHeapProperties{};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferResourceDesc{};
    bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferResourceDesc.Width = sizeInBytes;
    bufferResourceDesc.Height = 1;
    bufferResourceDesc.DepthOrArraySize = 1;
    bufferResourceDesc.MipLevels = 1;
    bufferResourceDesc.SampleDesc.Count = 1;
    bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&bufferResource)
    );
    assert(SUCCEEDED(hr));

    return bufferResource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height) {

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

    Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthClearValue,
        IID_PPV_ARGS(&resource)
    );
    assert(SUCCEEDED(hr));

    return resource;
}

void Engine::Initialize() {
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    assert(SUCCEEDED(hr));

    SetUnhandledExceptionFilter(ExportDump);
    InitializeLogger();
    createLogFile();
    Log("String\n");

    hwnd_ = CreateGameWindow();

    input_ = std::make_unique<Input>();
    input_->Initialize(GetModuleHandle(nullptr), hwnd_);

#ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        debugController->SetEnableGPUBasedValidation(TRUE);
    }
#endif

    // DirectX12の初期化
    bool isInitialized = dx12Device_.Initialize();
    assert(isInitialized);

    auto device = dx12Device_.GetDevice();
    auto commandList = dx12Device_.GetCommandList();
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
    bool isSwapChainInit = swapChain_.Initialize(dxgiFactory, commandQueue, hwnd_, kClientWidth, kClientHeight);
    assert(isSwapChainInit);

    // RTVの作成
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    rtvHandles_[0] = rtvHeapManager_.GetCPUDescriptorHandle(0);
    device->CreateRenderTargetView(swapChain_.GetBuffer(0).Get(), &rtvDesc, rtvHandles_[0]);

    rtvHandles_[1] = rtvHeapManager_.GetCPUDescriptorHandle(1);
    device->CreateRenderTargetView(swapChain_.GetBuffer(1).Get(), &rtvDesc, rtvHandles_[1]);

    depthStencilResource_ = CreateDepthStencilTextureResource(device, kClientWidth, kClientHeight);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(depthStencilResource_.Get().Get(), &dsvDesc, dsvHeapManager_.GetCPUDescriptorHandle(0));


	// ShaderCompilerの初期化
    shaderCompiler_ = std::make_unique<ShaderCompiler>();
    bool isShaderCompilerInit = shaderCompiler_->Initialize();
    assert(isShaderCompilerInit);


    // PipelineManagerの初期化
    pipelineManager_ = std::make_unique<PipelineManager>();
    pipelineManager_->Initialize(device, shaderCompiler_.get());


    // ビューポート
    viewport_.Width = kClientWidth;
    viewport_.Height = kClientHeight;
    viewport_.TopLeftX = 0;
    viewport_.TopLeftY = 0;
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    scissorRect_.left = 0;
    scissorRect_.right = kClientWidth;
    scissorRect_.top = 0;
    scissorRect_.bottom = kClientHeight;

    // オーディオ基盤の初期化
    hr = XAudio2Create(xAudio2_.GetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR);
    assert(SUCCEEDED(hr));
    hr = xAudio2_->CreateMasteringVoice(&masterVoice_);
    assert(SUCCEEDED(hr));

#ifdef USE_IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd_);
    ImGui_ImplDX12_Init(device.Get(), swapChain_.GetBufferCount(), rtvDesc.Format, srvHeapManager_.GetHeap(),
        srvHeapManager_.GetCPUDescriptorHandle(0), srvHeapManager_.GetGPUDescriptorHandle(0));
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Build();
#endif

    // ★GameSceneの生成と初期化
    gameScene_ = std::make_unique<GameScene>();
    gameScene_->Initialize(device.Get(), commandList.Get(), &srvHeapManager_, xAudio2_.Get());
}

void Engine::Run() {
    MSG msg{};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            Update();
            Draw();
        }
    }
}

void Engine::Update() {
    if (input_) {
        input_->Update();
    }

#ifdef USE_IMGUI
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
#endif

    // ★GameSceneの更新
    gameScene_->Update(kClientWidth, kClientHeight,input_.get());

#ifdef USE_IMGUI
    ImGui::Render();
#endif
}

void Engine::Draw() {
    auto commandList = dx12Device_.GetCommandList();
    auto commandAllocator = dx12Device_.GetCommandAllocator();
    auto commandQueue = dx12Device_.GetCommandQueue();

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

    pipelineManager_->Bind(commandList);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D12DescriptorHeap* descriptorHeaps[] = { srvHeapManager_.GetHeap() };
    commandList->SetDescriptorHeaps(1, descriptorHeaps);

    // ★GameSceneの描画呼び出し
    gameScene_->Draw(commandList.Get());

#ifdef USE_IMGUI
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
#endif

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

void Engine::Finalize() {
    // ★GameSceneの終了処理
    if (gameScene_) {
        gameScene_->Finalize();
        gameScene_.reset();
    }

    xAudio2_.Reset();

#ifdef USE_IMGUI
    ImGui_ImplWin32_Shutdown();
    ImGui_ImplDX12_Shutdown();
    ImGui::DestroyContext();
#endif

    CoUninitialize();
    CloseWindow(hwnd_);
}

