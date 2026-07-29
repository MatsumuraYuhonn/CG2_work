#include "GamePad.h"
#include <cstring>
#include <algorithm>
#include <cmath>

namespace MTEngine {

    namespace {
        // XInput標準のデッドゾーン定数
        constexpr SHORT kLeftStickDeadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
        constexpr SHORT kRightStickDeadZone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
        constexpr BYTE kTriggerDeadZone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    }

    void GamePad::Initialize() {
        // 接続状態を初期チェックしておく（コントローラー0番のみ）
        XINPUT_STATE state{};
        DWORD result = XInputGetState(0, &state);
        state_.isConnected = (result == ERROR_SUCCESS);
    }

    void GamePad::Update() {
        // 前フレームのボタン状態を保存
        state_.preButtons = state_.buttons;

        XINPUT_STATE xstate{};
        DWORD result = XInputGetState(0, &xstate);

        if (result != ERROR_SUCCESS) {
            // 未接続、または切断された場合は状態をクリア
            state_.isConnected = false;
            state_.buttons = 0;
            state_.leftStick = {};
            state_.rightStick = {};
            state_.leftTrigger = 0.0f;
            state_.rightTrigger = 0.0f;
            return;
        }

        state_.isConnected = true;

        const XINPUT_GAMEPAD& pad = xstate.Gamepad;
        state_.buttons = pad.wButtons;

        state_.leftStick.x = NormalizeStickAxis(pad.sThumbLX, kLeftStickDeadZone);
        state_.leftStick.y = NormalizeStickAxis(pad.sThumbLY, kLeftStickDeadZone);
        state_.rightStick.x = NormalizeStickAxis(pad.sThumbRX, kRightStickDeadZone);
        state_.rightStick.y = NormalizeStickAxis(pad.sThumbRY, kRightStickDeadZone);

        state_.leftTrigger = NormalizeTrigger(pad.bLeftTrigger, kTriggerDeadZone);
        state_.rightTrigger = NormalizeTrigger(pad.bRightTrigger, kTriggerDeadZone);
    }

    bool GamePad::IsConnected(int /*index*/) const {
        return state_.isConnected;
    }

    bool GamePad::PushButton(GamePadButton button, int /*index*/) const {
        WORD mask = static_cast<WORD>(button);
        return (state_.buttons & mask) != 0;
    }

    bool GamePad::TriggerButton(GamePadButton button, int /*index*/) const {
        WORD mask = static_cast<WORD>(button);
        return (state_.buttons & mask) && !(state_.preButtons & mask);
    }

    bool GamePad::ExitButton(GamePadButton button, int /*index*/) const {
        WORD mask = static_cast<WORD>(button);
        return !(state_.buttons & mask) && (state_.preButtons & mask);
    }

    const StickState& GamePad::GetLeftStick(int /*index*/) const {
        return state_.leftStick;
    }

    const StickState& GamePad::GetRightStick(int /*index*/) const {
        return state_.rightStick;
    }

    float GamePad::GetLeftTrigger(int /*index*/) const {
        return state_.leftTrigger;
    }

    float GamePad::GetRightTrigger(int /*index*/) const {
        return state_.rightTrigger;
    }

    void GamePad::SetVibration(float leftMotor, float rightMotor, int /*index*/) {
        leftMotor = std::clamp(leftMotor, 0.0f, 1.0f);
        rightMotor = std::clamp(rightMotor, 0.0f, 1.0f);

        XINPUT_VIBRATION vibration{};
        vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
        vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);
        XInputSetState(0, &vibration);
    }

    void GamePad::StopVibration(int /*index*/) {
        SetVibration(0.0f, 0.0f, 0);
    }

    float GamePad::NormalizeStickAxis(SHORT value, SHORT deadZone) {
        // デッドゾーン内は0として扱う
        if (value > -deadZone && value < deadZone) {
            return 0.0f;
        }

        // -32768〜32767 を -1.0f〜1.0f に正規化（デッドゾーン分を除去して滑らかにする）
        float normalized;
        if (value > 0) {
            normalized = static_cast<float>(value - deadZone) / static_cast<float>(32767 - deadZone);
        }
        else {
            normalized = static_cast<float>(value + deadZone) / static_cast<float>(32768 - deadZone);
        }

        return std::clamp(normalized, -1.0f, 1.0f);
    }

    float GamePad::NormalizeTrigger(BYTE value, BYTE deadZone) {
        if (value < deadZone) {
            return 0.0f;
        }
        float normalized = static_cast<float>(value - deadZone) / static_cast<float>(255 - deadZone);
        return std::clamp(normalized, 0.0f, 1.0f);
    }

}
