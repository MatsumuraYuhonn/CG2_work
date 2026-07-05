#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <windows.h>
#include <wrl.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

class Input {
public:
    Input() = default;
    ~Input();

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    // 初期化と更新
    void Initialize(HINSTANCE hInstance, HWND hwnd);
    void Update();

    // キー状態の判定ヘルパー
    bool PushKey(BYTE keyNumber) const; 
    bool TriggerKey(BYTE keyNumber) const;
    bool ExitKey(BYTE keyNumber) const;

private:
    Microsoft::WRL::ComPtr<IDirectInput8> directInput_ = nullptr;
    Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_ = nullptr;

    BYTE key_[256] = {};
    BYTE preKey_[256] = {};
};