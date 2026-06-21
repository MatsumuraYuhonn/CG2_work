#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")


extern HWND hwnd;

extern IDirectInput8* directInput;

extern IDirectInputDevice8* keyboard;

extern WNDCLASS w;

extern BYTE key[256];

extern HRESULT hr;

// preKeys
extern BYTE preKey[256];

void DeviceInputInitialize(HINSTANCE hInstance, HWND targetHwnd);
void DeviceInputUpdate();

bool Key(BYTE keyNumber);
