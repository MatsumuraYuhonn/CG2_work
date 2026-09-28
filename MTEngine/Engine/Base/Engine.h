#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgidebug.h>
#include <memory>
#include <wrl.h>

#include "MTEngine/Engine/Audio/AudioManager.h"
#include "MTEngine/Engine/Base/CrashHandler.h"
#include "MTEngine/Engine/Base/Dx12Device.h"
#include "MTEngine/Engine/Base/ShaderCompiler.h"
#include "MTEngine/Engine/Base/SwapChain.h"
#include "MTEngine/Engine/Base/Window.h"
#include "MTEngine/Engine/Debug/DebugCamera.h"
#include "MTEngine/Engine/Debug/ImGuiManager.h"
#include "MTEngine/Engine/Editor/BaseEditor.h"
#include "MTEngine/Engine/Graphics/PipelineManager.h"
#include "MTEngine/Engine/Graphics/Renderer.h"
#include "MTEngine/Engine/Input/Input.h"

#pragma comment(lib, "dxcompiler.lib")

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

    // Sprite / TextureManager が共通で利用するアップロードバッファ生成関数。
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(
        Microsoft::WRL::ComPtr<ID3D12Device> device,
        size_t sizeInBytes);

    inline size_t AlignForConstantBuffer(size_t size) {
        return (size + 255) & ~255;
    }

    // DirectX 12 の初期化、メインループ、描画フレームを管理する基底クラス。
    // 新しいゲームではこのクラスを継承し、On... の各関数だけを実装する。
    class Engine {
    public:
        Engine() = default;
        virtual ~Engine() = default;

        void Initialize();
        void Run();
        void Finalize();

    protected:
        virtual void OnInitialize() {}
        virtual void OnUpdate() {}
        virtual void OnDraw() {}
        virtual void OnFinalize() {}

        Input* GetInput() const { return input_.get(); }
        Renderer* GetRenderer() const { return renderer_.get(); }
        AudioManager* GetAudioManager() const { return audioManager_.get(); }
        DebugCamera& GetDebugCamera() { return debugCamera_; }
        ID3D12Device* GetDevice() const { return dx12Device_.GetDevice().Get(); }
        ID3D12GraphicsCommandList* GetCommandList() const {
            return renderer_ ? renderer_->GetCommandList() : nullptr;
        }

    private:
        void Update();
        void Draw();

        HWND hwnd_ = nullptr;
        bool isInitialized_ = false;
        std::unique_ptr<Input> input_;

        // 最後に破棄されるよう、DirectX オブジェクトより前に宣言する。
        D3DResourceLeakChecker leakCheck_;
        Dx12Device dx12Device_;
        SwapChain swapChain_;
        std::unique_ptr<ShaderCompiler> shaderCompiler_;
        std::unique_ptr<PipelineManager> pipelineManager_;
        std::unique_ptr<Renderer> renderer_;
        std::unique_ptr<ImGuiManager> imGuiManager_;
        std::unique_ptr<BaseEditor> editor_;
        std::unique_ptr<AudioManager> audioManager_;
        DebugCamera debugCamera_;
    };

}
