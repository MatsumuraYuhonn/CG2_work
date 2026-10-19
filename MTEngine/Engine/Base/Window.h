#pragma once

#include <Windows.h>
#include <cstdint>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);
#endif

namespace MTEngine {

    // Win32 ウィンドウの生成、破棄、メッセージ処理を管理する。
    class Window final {
    public:
        static constexpr int32_t kClientWidth = 1280;
        static constexpr int32_t kClientHeight = 720;

        Window() = default;
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool Initialize();
        void Finalize();

        // 保留中のメッセージを処理し、終了要求を受けた場合は false を返す。
        bool ProcessMessages();

        HWND GetHwnd() const { return hwnd_; }
        float ConsumeMouseWheelDelta();

    private:
        static LRESULT CALLBACK WindowProc(
            HWND hwnd,
            UINT msg,
            WPARAM wparam,
            LPARAM lparam);
        LRESULT HandleMessage(
            HWND hwnd,
            UINT msg,
            WPARAM wparam,
            LPARAM lparam);

        static constexpr const wchar_t* kWindowClassName = L"CG2WindowClass";
        static constexpr const wchar_t* kWindowTitle = L"LE2A_25_マツムラ_ユホン";

        HWND hwnd_ = nullptr;
        HINSTANCE instance_ = nullptr;
        float mouseWheelDelta_ = 0.0f;
        bool isClassRegistered_ = false;
    };

}
