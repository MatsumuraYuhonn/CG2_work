#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <windows.h>
#include <wrl.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

// キー入力管理クラス
class Input {
public:
    Input() = default;
    ~Input();

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    // DirectInputの初期化
    void Initialize(HINSTANCE hInstance, HWND hwnd);
    // キー入力状態の更新
    void Update();

    // キーが押されているか
    bool PushKey(BYTE keyNumber) const;
    // キーがトリガーされた（押された瞬間）か
    bool TriggerKey(BYTE keyNumber) const;
    // キーが離された瞬間か
    bool ExitKey(BYTE keyNumber) const;

private:
    Microsoft::WRL::ComPtr<IDirectInput8> directInput_ = nullptr;
    Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_ = nullptr;

    BYTE key_[256] = {};     // 現在のフレームの入力状態
    BYTE preKey_[256] = {};  // 前のフレームの入力状態
};