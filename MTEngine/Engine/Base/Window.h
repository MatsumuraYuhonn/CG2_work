#pragma once
#include <Windows.h>
#include <cstdint>

#ifdef USE_IMGUI

#include"externals/imgui/imgui.h"
#include"externals/imgui/imgui_impl_dx12.h"
#include"externals/imgui/imgui_impl_win32.h"



extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif

const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;

// ウィンドウプロシージャの宣言
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

// ウィンドウを作成・表示する関数の宣言
HWND CreateGameWindow();
