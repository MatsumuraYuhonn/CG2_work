#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif

namespace MTEngine {

    class ImGuiManager {
    public:
        ImGuiManager() = default;
        ~ImGuiManager() = default;

        /// <summary>
        /// ImGuiの初期化
        /// </summary>
        void Initialize(
            HWND hwnd,
            ID3D12Device* device,
            UINT numFramesInPipeline,
            DXGI_FORMAT rtvFormat,
            ID3D12DescriptorHeap* srvHeap,
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle
        );

        /// <summary>
        /// フレーム開始処理（Updateの冒頭で呼び出す）
        /// </summary>
        void BeginFrame();

        /// <summary>
        /// フレーム終了処理（Updateの末尾で呼び出す）
        /// </summary>
        void EndFrame();

        /// <summary>
        /// 描画実行（Draw内で呼び出す）
        /// </summary>
        void Draw(ID3D12GraphicsCommandList* commandList);

        /// <summary>
        /// 終了処理
        /// </summary>
        void Finalize();
    };

}
