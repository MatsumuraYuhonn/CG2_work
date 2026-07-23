#pragma once
#include <Windows.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <wrl.h>
#include <memory>
#include <xaudio2.h>
#include <cassert>

#include "MTEngine/Engine/Debug/Logger.h"
#include "Window.h"
#include "MTEngine/Engine/Math/Vector.h"
#include "MTEngine/Engine/Input/Input.h"
#include "MTEngine/Engine/Audio/Sound.h"
#include "Dx12Device.h"
#include "MTEngine/Engine/Math/Transform.h"
#include "CrashHandler.h"
#include "ShaderCompiler.h"
#include "SwapChain.h"
#include "MTEngine/Engine/Graphics/DescriptorHeapManager.h"
#include "MTEngine/Engine/Graphics/PipelineManager.h"
#include "MTEngine/Game/Scene/GameScene.h" 
#include "MTEngine/Engine/Debug/ImGuiManager.h"
#include "MTEngine/Engine/Audio/AudioManager.h"
#include "MTEngine/Engine/Graphics/Renderer.h"


#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

#pragma comment(lib, "dxcompiler.lib")

// DirectX12のリソースリークをデバッグビルド時に検知するヘルパークラス
namespace MTEngine {

    struct D3DResourceLeakChecker {
        ~D3DResourceLeakChecker() {
            Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
                debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
                debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
                debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
            }
        }
    };

}

// ID3D12Resourceをラップして管理するクラス
namespace MTEngine {

    class ResourceObject {
    public:
        ResourceObject() : resource_(nullptr) {}
        ResourceObject(std::nullptr_t) : resource_(nullptr) {}
        ResourceObject(Microsoft::WRL::ComPtr<ID3D12Resource> resource) : resource_(resource) {}
        ~ResourceObject() {}

        Microsoft::WRL::ComPtr<ID3D12Resource> Get() { return resource_; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    };

    // グローバル補助関数
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes);
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height);
    inline size_t AlignForConstantBuffer(size_t size) {
        return (size + 255) & ~255; // 256バイトアライメント
    }

    // ゲームエンジンのメインクラス。全システムの初期化・更新・描画を統括する
    class Engine {
    public:
        Engine() = default;
        ~Engine() = default;

        // 基盤ライフサイクル管理
        void Initialize();
        void Run();
        void Finalize();

    private:
        void Update();
        void Draw();

        HWND hwnd_ = nullptr;
        std::unique_ptr<Input> input_;

        // 1. リークチェック（最上部で定義し、一番最後に解放されるようにする）
        D3DResourceLeakChecker leakCheck_;

        // 2. Windows / DirectX12 基盤
        Dx12Device dx12Device_;
        SwapChain swapChain_;

        // 3. パイプライン・シェーダ・レンダラー関連
        std::unique_ptr<ShaderCompiler> shaderCompiler_;
        std::unique_ptr<PipelineManager> pipelineManager_;
        std::unique_ptr<Renderer> renderer_;
        std::unique_ptr<ImGuiManager> imGuiManager_;

        // 4. オーディオ基盤
        std::unique_ptr<AudioManager> audioManager_;

        // 5. ゲームシーン
        std::unique_ptr<GameScene> gameScene_;
    };

}
