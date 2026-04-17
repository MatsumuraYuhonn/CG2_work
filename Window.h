#pragma once
#include <Windows.h>
#include <cstdint>

const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;

// ウィンドウプロシージャの宣言
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

// ウィンドウを作成・表示する関数の宣言
HWND CreateGameWindow();
