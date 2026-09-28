#include "Engine.h"

#include <array>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "MTEngine/Engine/Debug/Logger.h"

namespace MTEngine {
    namespace {

        bool ConfigureWorkingDirectory() {
            std::array<wchar_t, 32768> executablePathBuffer{};
            const DWORD executablePathLength = GetModuleFileNameW(
                nullptr,
                executablePathBuffer.data(),
                static_cast<DWORD>(executablePathBuffer.size()));
            if (executablePathLength == 0 ||
                executablePathLength >= executablePathBuffer.size()) {
                return false;
            }

            const std::filesystem::path executableDirectory =
                std::filesystem::path(std::wstring(
                    executablePathBuffer.data(),
                    executablePathLength)).parent_path();
            std::error_code error;
            const std::filesystem::path currentDirectory =
                std::filesystem::current_path(error);
            const std::array<std::filesystem::path, 5> candidates = {
                currentDirectory,
                executableDirectory,
                executableDirectory.parent_path(),
                executableDirectory.parent_path().parent_path(),
                executableDirectory.parent_path().parent_path().parent_path(),
            };

            for (const std::filesystem::path& candidate : candidates) {
                if (candidate.empty()) {
                    continue;
                }
                const std::filesystem::path shaderDirectory =
                    candidate / "MTEngine" / "Assets" / "Shaders";
                if (!std::filesystem::is_directory(shaderDirectory, error)) {
                    continue;
                }
                std::filesystem::current_path(candidate, error);
                return !error;
            }
            return false;
        }

    }

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(
        Microsoft::WRL::ComPtr<ID3D12Device> device,
        size_t sizeInBytes) {
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

        Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource;
        const HRESULT result = device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferResourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&bufferResource));
        assert(SUCCEEDED(result));
        return bufferResource;
    }

    void Engine::Initialize() {
        if (!ConfigureWorkingDirectory()) {
            MessageBoxW(
                nullptr,
                L"MTEngine/Assets/Shaders が見つかりません。\n"
                L"実行ファイルと MTEngine フォルダーの配置を確認してください。",
                L"MTEngine - Resource Error",
                MB_OK | MB_ICONERROR);
            ExitProcess(EXIT_FAILURE);
        }

        const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        assert(SUCCEEDED(comResult));

        SetUnhandledExceptionFilter(ExportDump);
        InitializeLogger();
        createLogFile();
        Log("MTEngine initialized\n");

        hwnd_ = CreateGameWindow();
        input_ = std::make_unique<Input>();
        input_->Initialize(GetModuleHandle(nullptr), hwnd_);

#ifdef _DEBUG
        Microsoft::WRL::ComPtr<ID3D12Debug1> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
            debugController->SetEnableGPUBasedValidation(TRUE);
        }
#endif

        const bool isDeviceInitialized = dx12Device_.Initialize();
        assert(isDeviceInitialized);

        const auto device = dx12Device_.GetDevice();
#ifdef _DEBUG
        Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
            D3D12_MESSAGE_ID denyIds[] = {
                D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
            };
            D3D12_MESSAGE_SEVERITY severities[] = {
                D3D12_MESSAGE_SEVERITY_INFO
            };
            D3D12_INFO_QUEUE_FILTER filter{};
            filter.DenyList.NumIDs = _countof(denyIds);
            filter.DenyList.pIDList = denyIds;
            filter.DenyList.NumSeverities = _countof(severities);
            filter.DenyList.pSeverityList = severities;
            infoQueue->PushStorageFilter(&filter);
        }
#endif

        const bool isSwapChainInitialized = swapChain_.Initialize(
            dx12Device_.GetDxgiFactory(),
            dx12Device_.GetCommandQueue(),
            hwnd_,
            kClientWidth,
            kClientHeight);
        assert(isSwapChainInitialized);

        shaderCompiler_ = std::make_unique<ShaderCompiler>();
        const bool isShaderCompilerInitialized = shaderCompiler_->Initialize();
        assert(isShaderCompilerInitialized);
        if (!isShaderCompilerInitialized) {
            MessageBoxW(
                hwnd_,
                L"シェーダーコンパイラーの初期化に失敗しました。",
                L"MTEngine - Shader Compiler Error",
                MB_OK | MB_ICONERROR);
            ExitProcess(EXIT_FAILURE);
        }

        pipelineManager_ = std::make_unique<PipelineManager>();
        pipelineManager_->Initialize(device, shaderCompiler_.get());

        renderer_ = std::make_unique<Renderer>();
        renderer_->Initialize(
            &dx12Device_,
            &swapChain_,
            pipelineManager_.get(),
            kClientWidth,
            kClientHeight);

        audioManager_ = std::make_unique<AudioManager>();
        audioManager_->Initialize();

        imGuiManager_ = std::make_unique<ImGuiManager>();
        imGuiManager_->Initialize(
            hwnd_,
            device.Get(),
            swapChain_.GetBufferCount(),
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            renderer_->GetSrvHeapManager()->GetHeap(),
            renderer_->GetSrvHeapManager()->GetCPUDescriptorHandle(0),
            renderer_->GetSrvHeapManager()->GetGPUDescriptorHandle(0));

        debugCamera_.Initialize();
        editor_ = std::make_unique<BaseEditor>();
        editor_->Initialize(
            renderer_->GetSceneTextureHandle(),
            &debugCamera_);
        isInitialized_ = true;
        OnInitialize();
    }

    void Engine::Run() {
        assert(isInitialized_);
        MSG message{};
        while (message.message != WM_QUIT) {
            if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&message);
                DispatchMessage(&message);
            } else {
                Update();
                Draw();
            }
        }
    }

    void Engine::Update() {
        input_->Update();
        imGuiManager_->BeginFrame();
        editor_->Update();
        OnUpdate();
#ifdef USE_IMGUI
        debugCamera_.Update(input_.get(), editor_->IsSceneViewFocused());
#else
        debugCamera_.Update(input_.get(), true);
#endif
        imGuiManager_->EndFrame();
    }

    void Engine::Draw() {
        renderer_->BeginFrame();
        renderer_->BeginScene();
        OnDraw();
        renderer_->EndScene();

#ifdef USE_IMGUI
        imGuiManager_->Draw(renderer_->GetCommandList());
#else
        renderer_->CopySceneToBackBuffer();
#endif

        renderer_->EndFrame();
    }

    void Engine::Finalize() {
        if (!isInitialized_) {
            return;
        }

        OnFinalize();
        editor_.reset();
        audioManager_->Finalize();
        audioManager_.reset();

        imGuiManager_->Finalize();
        imGuiManager_.reset();
        renderer_.reset();
        pipelineManager_.reset();
        shaderCompiler_.reset();
        input_.reset();

        CloseWindow(hwnd_);
        hwnd_ = nullptr;
        CoUninitialize();
        isInitialized_ = false;
    }

}
