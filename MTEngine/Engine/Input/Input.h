#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <windows.h>
#include <wrl.h>
#include <memory>

#include "GamePad.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

// キー入力管理クラス（キーボード + ゲームパッド）
namespace MTEngine {

    class Input {
    public:
        Input() = default;
        ~Input();

        Input(const Input&) = delete;
        Input& operator=(const Input&) = delete;

        // DirectInput・XInputの初期化
        void Initialize(HINSTANCE hInstance, HWND hwnd);
        // 入力状態の更新（キーボード・ゲームパッド両方）
        void Update();

        // キーが押されているか
        bool PushKey(BYTE keyNumber) const;
        // キーがトリガーされた（押された瞬間）か
        bool TriggerKey(BYTE keyNumber) const;
        // キーが離された瞬間か
        bool ExitKey(BYTE keyNumber) const;

        // ゲームパッドへのアクセス（複数台対応、index省略時は0番目）
        GamePad* GetGamePad() { return gamePad_.get(); }
        const GamePad* GetGamePad() const { return gamePad_.get(); }

    private:
        Microsoft::WRL::ComPtr<IDirectInput8> directInput_ = nullptr;
        Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_ = nullptr;

        BYTE key_[256] = {};     // 現在のフレームの入力状態
        BYTE preKey_[256] = {};  // 前のフレームの入力状態

        std::unique_ptr<GamePad> gamePad_;
    };

}