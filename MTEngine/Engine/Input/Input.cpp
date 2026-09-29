#include "Input.h"
#include <cassert>
#include <algorithm> 
#include <cstring>
#include <iterator>


namespace MTEngine {

    Input::~Input() {

        if (keyboard_) {
            keyboard_->Unacquire();
        }
    }

    void Input::Initialize(HINSTANCE hInstance, HWND hwnd) {
        HRESULT hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION,
            IID_IDirectInput8, (void**)&directInput_, nullptr);
        assert(SUCCEEDED(hr));

        hr = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
        assert(SUCCEEDED(hr));

        hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
        assert(SUCCEEDED(hr));

        hr = keyboard_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
        assert(SUCCEEDED(hr));

        // 初回クリア
        std::fill(std::begin(key_), std::end(key_), 0);
        std::fill(std::begin(preKey_), std::end(preKey_), 0);

        // ゲームパッド（XInput）の初期化
        gamePad_ = std::make_unique<GamePad>();
        gamePad_->Initialize();
    }

    void Input::Update() {
        // 前回のキー状態をコピー
        std::memcpy(preKey_, key_, sizeof(key_));

        if (keyboard_) {
            HRESULT hr = keyboard_->GetDeviceState(sizeof(key_), key_);
            if (FAILED(hr)) {
                // フォーカスが外れた場合は再取得を試みる
                keyboard_->Acquire();
                std::memset(key_, 0, sizeof(key_));
            }
        }

        // ゲームパッドの入力状態を更新
        if (gamePad_) {
            gamePad_->Update();
        }
    }

    bool Input::PushKey(BYTE keyNumber) const {
        return key_[keyNumber] & 0x80;
    }

    bool Input::TriggerKey(BYTE keyNumber) const {
        return (key_[keyNumber] & 0x80) && !(preKey_[keyNumber] & 0x80);
    }

    bool Input::ExitKey(BYTE keyNumber) const {
        return !(key_[keyNumber] & 0x80) && (preKey_[keyNumber] & 0x80);
    }

}
