#include "Window.h"

namespace MTEngine {

    Window::~Window() {
        Finalize();
    }

    bool Window::Initialize() {
        if (hwnd_ != nullptr) {
            return true;
        }

        instance_ = GetModuleHandle(nullptr);

        WNDCLASS windowClass{};
        windowClass.lpfnWndProc = WindowProc;
        windowClass.lpszClassName = kWindowClassName;
        windowClass.hInstance = instance_;
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

        if (RegisterClass(&windowClass) == 0) {
            return false;
        }
        isClassRegistered_ = true;

        constexpr DWORD kFixedWindowStyle =
            WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        RECT windowRect = { 0, 0, kClientWidth, kClientHeight };
        if (!AdjustWindowRect(&windowRect, kFixedWindowStyle, FALSE)) {
            Finalize();
            return false;
        }

        hwnd_ = CreateWindow(
            kWindowClassName,
            kWindowTitle,
            kFixedWindowStyle,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top,
            nullptr,
            nullptr,
            instance_,
            this);
        if (hwnd_ == nullptr) {
            Finalize();
            return false;
        }

        ShowWindow(hwnd_, SW_SHOW);
        return true;
    }

    void Window::Finalize() {
        if (hwnd_ != nullptr) {
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
        }

        if (isClassRegistered_) {
            UnregisterClass(kWindowClassName, instance_);
            isClassRegistered_ = false;
        }

        instance_ = nullptr;
        mouseWheelDelta_ = 0.0f;
    }

    bool Window::ProcessMessages() {
        MSG message{};
        while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                return false;
            }

            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        return true;
    }

    float Window::ConsumeMouseWheelDelta() {
        const float delta = mouseWheelDelta_;
        mouseWheelDelta_ = 0.0f;
        return delta;
    }

    LRESULT CALLBACK Window::WindowProc(
        HWND hwnd,
        UINT message,
        WPARAM wparam,
        LPARAM lparam) {
        Window* window = reinterpret_cast<Window*>(
            GetWindowLongPtr(hwnd, GWLP_USERDATA));

        if (message == WM_NCCREATE) {
            const CREATESTRUCT* createStruct =
                reinterpret_cast<const CREATESTRUCT*>(lparam);
            window = static_cast<Window*>(createStruct->lpCreateParams);
            SetWindowLongPtr(
                hwnd,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(window));
        }

        if (window != nullptr) {
            return window->HandleMessage(hwnd, message, wparam, lparam);
        }

        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    LRESULT Window::HandleMessage(
        HWND hwnd,
        UINT message,
        WPARAM wparam,
        LPARAM lparam) {
        if (message == WM_MOUSEWHEEL) {
            mouseWheelDelta_ +=
                static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam)) /
                WHEEL_DELTA;
            return 0;
        }

#ifdef USE_IMGUI
        if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam)) {
            return TRUE;
        }
#endif

        if (message == WM_DESTROY) {
            hwnd_ = nullptr;
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProc(hwnd, message, wparam, lparam);
    }

}
