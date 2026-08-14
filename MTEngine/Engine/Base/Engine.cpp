#include "Engine.h"
#include <cassert>
#include <cmath>

namespace MTEngine {

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

        // ShaderCompilerの初期化
        shaderCompiler_ = std::make_unique<ShaderCompiler>();
        bool isShaderCompilerInit = shaderCompiler_->Initialize();
        assert(isShaderCompilerInit);

        // PipelineManagerの初期化
        pipelineManager_ = std::make_unique<PipelineManager>();
        pipelineManager_->Initialize(device, shaderCompiler_.get());

        // Rendererの初期化
        renderer_ = std::make_unique<Renderer>();
        renderer_->Initialize(&dx12Device_, &swapChain_, pipelineManager_.get(), kClientWidth, kClientHeight);

        audioManager_ = std::make_unique<AudioManager>();
        audioManager_->Initialize();

        // ImGuiManagerの初期化
        imGuiManager_ = std::make_unique<ImGuiManager>();
        imGuiManager_->Initialize(
            hwnd_,
            device.Get(),
            swapChain_.GetBufferCount(),
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            renderer_->GetSrvHeapManager()->GetHeap(),
            renderer_->GetSrvHeapManager()->GetCPUDescriptorHandle(0),
            renderer_->GetSrvHeapManager()->GetGPUDescriptorHandle(0)
        );

        debugCamera_.Initialize();

        const bool isGameSceneInitialized = gameScene_.Initialize(
            device,
            commandList,
            renderer_->GetSrvHeapManager(),
            "MTEngine/Assets/Resources/Stages/sample_stage.csv");
        assert(isGameSceneInitialized);

        editor_ = std::make_unique<Editor>();
        editor_->Initialize(renderer_->GetSceneTextureHandle(), &gameScene_, &debugCamera_);

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

        // ImGuiのフレーム開始
        imGuiManager_->BeginFrame();

        editor_->Update();
        gameScene_.SetSelectedCellIndex(editor_->GetSelectedTileIndex());
        gameScene_.UpdatePlayer(input_.get());
        debugCamera_.Update(input_.get(), editor_->IsSceneViewFocused());

        imGuiManager_->EndFrame();
    }

    void Engine::Draw() {
        // フレーム開始処理 (クリア、バリア遷移、ビューポート設定、パイプラインバインド等)
        renderer_->BeginFrame();

        auto commandList = renderer_->GetCommandList();

        renderer_->BeginScene();
        gameScene_.Draw(
            commandList,
            debugCamera_.GetViewMatrix(),
            debugCamera_.GetProjectionMatrix());
        renderer_->EndScene();

        // ImGuiの描画
        imGuiManager_->Draw(commandList);

        // フレーム終了処理 (バリア遷移、ExecuteCommandLists、Present、Wait)
        renderer_->EndFrame();
    }

    void Engine::Finalize() {
    
        if (audioManager_) {
            audioManager_->Finalize();
            audioManager_.reset();
        }

        editor_.reset();

        if (imGuiManager_) {
            imGuiManager_->Finalize();
            imGuiManager_.reset();
        }

        CoUninitialize();
        CloseWindow(hwnd_);
    }

}
