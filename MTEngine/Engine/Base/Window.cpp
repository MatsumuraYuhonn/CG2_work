#include "Window.h"

namespace MTEngine {

	float gMouseWheelDelta = 0.0f;

	void AddMouseWheelDelta(float delta) {
		gMouseWheelDelta += delta;
	}

	float ConsumeMouseWheelDelta() {
		const float delta = gMouseWheelDelta;
		gMouseWheelDelta = 0.0f;
		return delta;
	}

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		if (msg == WM_MOUSEWHEEL) {
			AddMouseWheelDelta(static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam)) / WHEEL_DELTA);
			return 0;
		}

#ifdef USE_IMGUI

		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
			return true;
		}

#endif

		switch (msg) {

		case WM_DESTROY:

			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProc(hwnd, msg, wparam, lparam);
	}


	HWND CreateGameWindow() {
		constexpr DWORD kFixedWindowStyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);

		WNDCLASS wc{};

		// ウィンドウプロシージャを設定
		wc.lpfnWndProc = WindowProc;
		wc.lpszClassName = L"CG2WindowClass";
		wc.hInstance = GetModuleHandle(nullptr);
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

		RegisterClass(&wc);

		// ウィンドウサイズの計算
		RECT wrc = { 0, 0, kClientWidth, kClientHeight };

		AdjustWindowRect(&wrc, kFixedWindowStyle, false);

		// ウィンドウの生成
		HWND hwnd = CreateWindow(
			wc.lpszClassName,
			L"LE2A_25_マツムラ_ユホン",
			kFixedWindowStyle,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			wrc.right - wrc.left,
			wrc.bottom - wrc.top,
			nullptr,
			nullptr,
			wc.hInstance,
			nullptr);

		// ウィンドウを表示する
		ShowWindow(hwnd, SW_SHOW);

		return hwnd;
	}

}
