#include "Window.h"

namespace MTEngine {

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

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

		WNDCLASS wc{};

		// ウィンドウプロシージャを設定
		wc.lpfnWndProc = WindowProc;
		wc.lpszClassName = L"CG2WindowClass";
		wc.hInstance = GetModuleHandle(nullptr);
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

		RegisterClass(&wc);

		// ウィンドウサイズの計算
		RECT wrc = { 0, 0, kClientWidth, kClientHeight };

		AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

		// ウィンドウの生成
		HWND hwnd = CreateWindow(
			wc.lpszClassName,
			L"CG2",
			WS_OVERLAPPEDWINDOW,
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
