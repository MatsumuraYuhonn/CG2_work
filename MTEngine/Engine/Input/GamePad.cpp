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
        // 接続状態を初期チェックしておく
        for (int i = 0; i < kMaxControllers; ++i) {
            XINPUT_STATE state{};
            DWORD result = XInputGetState(i, &state);
            states_[i].isConnected = (result == ERROR_SUCCESS);
        }
    }

    void GamePad::Update() {
        for (int i = 0; i < kMaxControllers; ++i) {
            auto& s = states_[i];

            // 前フレームのボタン状態を保存
            s.preButtons = s.buttons;

            XINPUT_STATE xstate{};
            DWORD result = XInputGetState(i, &xstate);

            if (result != ERROR_SUCCESS) {
                // 未接続、または切断された場合は状態をクリア
                s.isConnected = false;
                s.buttons = 0;
                s.leftStick = {};
                s.rightStick = {};
                s.leftTrigger = 0.0f;
                s.rightTrigger = 0.0f;
                continue;
            }

            s.isConnected = true;

            const XINPUT_GAMEPAD& pad = xstate.Gamepad;
            s.buttons = pad.wButtons;

            s.leftStick.x = NormalizeStickAxis(pad.sThumbLX, kLeftStickDeadZone);
            s.leftStick.y = NormalizeStickAxis(pad.sThumbLY, kLeftStickDeadZone);
            s.rightStick.x = NormalizeStickAxis(pad.sThumbRX, kRightStickDeadZone);
            s.rightStick.y = NormalizeStickAxis(pad.sThumbRY, kRightStickDeadZone);

            s.leftTrigger = NormalizeTrigger(pad.bLeftTrigger, kTriggerDeadZone);
            s.rightTrigger = NormalizeTrigger(pad.bRightTrigger, kTriggerDeadZone);
        }
    }

    bool GamePad::IsConnected(int index) const {
        if (index < 0 || index >= kMaxControllers) return false;
        return states_[index].isConnected;
    }

    bool GamePad::PushButton(GamePadButton button, int index) const {
        if (index < 0 || index >= kMaxControllers) return false;
        WORD mask = static_cast<WORD>(button);
        return (states_[index].buttons & mask) != 0;
    }

    bool GamePad::TriggerButton(GamePadButton button, int index) const {
        if (index < 0 || index >= kMaxControllers) return false;
        WORD mask = static_cast<WORD>(button);
        const auto& s = states_[index];
        return (s.buttons & mask) && !(s.preButtons & mask);
    }

    bool GamePad::ExitButton(GamePadButton button, int index) const {
        if (index < 0 || index >= kMaxControllers) return false;
        WORD mask = static_cast<WORD>(button);
        const auto& s = states_[index];
        return !(s.buttons & mask) && (s.preButtons & mask);
    }

    const StickState& GamePad::GetLeftStick(int index) const {
        static StickState empty{};
        if (index < 0 || index >= kMaxControllers) return empty;
        return states_[index].leftStick;
    }

    const StickState& GamePad::GetRightStick(int index) const {
        static StickState empty{};
        if (index < 0 || index >= kMaxControllers) return empty;
        return states_[index].rightStick;
    }

    float GamePad::GetLeftTrigger(int index) const {
        if (index < 0 || index >= kMaxControllers) return 0.0f;
        return states_[index].leftTrigger;
    }

    float GamePad::GetRightTrigger(int index) const {
        if (index < 0 || index >= kMaxControllers) return 0.0f;
        return states_[index].rightTrigger;
    }

    void GamePad::SetVibration(float leftMotor, float rightMotor, int index) {
        if (index < 0 || index >= kMaxControllers) return;

        leftMotor = std::clamp(leftMotor, 0.0f, 1.0f);
        rightMotor = std::clamp(rightMotor, 0.0f, 1.0f);

        XINPUT_VIBRATION vibration{};
        vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
        vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);
        XInputSetState(index, &vibration);
    }

    void GamePad::StopVibration(int index) {
        SetVibration(0.0f, 0.0f, index);
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
