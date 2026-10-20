#include "ImGuiManager.h"

namespace MTEngine {

    void ImGuiManager::Initialize(
        HWND hwnd,
        ID3D12Device* device,
        UINT numFramesInPipeline,
        DXGI_FORMAT rtvFormat,
        ID3D12DescriptorHeap* srvHeap,
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle
    ) {
#ifdef USE_IMGUI
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 3.0f;
        style.FrameRounding = 3.0f;
        style.GrabRounding = 3.0f;
        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.WindowPadding = ImVec2(10.0f, 8.0f);
        style.ItemSpacing = ImVec2(8.0f, 6.0f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.11f, 0.14f, 1.0f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.09f, 0.12f, 1.0f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.14f, 0.18f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.19f, 0.25f, 1.0f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.19f, 0.24f, 1.0f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.16f, 0.29f, 0.40f, 1.0f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.21f, 0.40f, 0.55f, 1.0f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.13f, 0.46f, 0.67f, 1.0f);
#ifdef IMGUI_HAS_DOCK
        style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.055f, 0.065f, 0.08f, 1.0f);
#endif

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX12_Init(
            device,
            numFramesInPipeline,
            rtvFormat,
            srvHeap,
            cpuHandle,
            gpuHandle
        );

        ImGuiIO& io = ImGui::GetIO();

#ifdef IMGUI_HAS_DOCK
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

        io.Fonts->Build();
#else
        (void)hwnd;
        (void)device;
        (void)numFramesInPipeline;
        (void)rtvFormat;
        (void)srvHeap;
        (void)cpuHandle;
        (void)gpuHandle;
#endif
    }

    void ImGuiManager::BeginFrame() {
#ifdef USE_IMGUI
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

#ifdef IMGUI_HAS_DOCK
        ImGuiID dockspaceID = ImGui::GetID("MainDockSpace");

        ImGui::DockSpaceOverViewport(
            dockspaceID,
            ImGui::GetMainViewport()
        );
#endif
#endif
    }

    void ImGuiManager::EndFrame() {
#ifdef USE_IMGUI
        ImGui::Render();
#endif
    }

    void ImGuiManager::Draw(ID3D12GraphicsCommandList* commandList) {
#ifdef USE_IMGUI
        if (commandList) {
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
        }
#else
        (void)commandList;
#endif
    }

    void ImGuiManager::Finalize() {
#ifdef USE_IMGUI
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
#endif
    }

}
