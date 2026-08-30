#include "GamePad.h"
#include <array>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace MTEngine {

    namespace {
        // XInput標準のデッドゾーン定数
        constexpr SHORT kLeftStickDeadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
        constexpr SHORT kRightStickDeadZone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
        constexpr BYTE kTriggerDeadZone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

        bool HasInput(const XINPUT_GAMEPAD& gamePad) {
            return gamePad.wButtons != 0 ||
                gamePad.bLeftTrigger >= kTriggerDeadZone ||
                gamePad.bRightTrigger >= kTriggerDeadZone ||
                std::abs(static_cast<int>(gamePad.sThumbLX)) >= kLeftStickDeadZone ||
                std::abs(static_cast<int>(gamePad.sThumbLY)) >= kLeftStickDeadZone ||
                std::abs(static_cast<int>(gamePad.sThumbRX)) >= kRightStickDeadZone ||
                std::abs(static_cast<int>(gamePad.sThumbRY)) >= kRightStickDeadZone;
        }
    }

    void GamePad::Initialize() {
        state_ = {};
        activeUserIndex_ = XUSER_MAX_COUNT;
        Update();
    }

    void GamePad::Update() {
        // 前フレームのボタン状態を保存
        state_.preButtons = state_.buttons;
        state_.preRightTrigger = state_.rightTrigger;

        std::array<XINPUT_STATE, XUSER_MAX_COUNT> xinputStates{};
        std::array<bool, XUSER_MAX_COUNT> connectedUsers{};
        for (DWORD userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
            connectedUsers[userIndex] =
                XInputGetState(userIndex, &xinputStates[userIndex]) == ERROR_SUCCESS;
        }

        DWORD connectedUserIndex = XUSER_MAX_COUNT;
        if (activeUserIndex_ < XUSER_MAX_COUNT && connectedUsers[activeUserIndex_]) {
            connectedUserIndex = activeUserIndex_;
        }

        // Windows can expose an idle or virtual XInput device before the
        // controller the player is actually using. Prefer whichever connected
        // device currently has meaningful input.
        for (DWORD userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
            if (!connectedUsers[userIndex] ||
                !HasInput(xinputStates[userIndex].Gamepad)) {
                continue;
            }
            if (connectedUserIndex == XUSER_MAX_COUNT ||
                !HasInput(xinputStates[connectedUserIndex].Gamepad)) {
                connectedUserIndex = userIndex;
            }
        }

        // Before the first input, select the device whose state changed most
        // recently. This avoids defaulting to a dormant slot zero device.
        if (connectedUserIndex == XUSER_MAX_COUNT) {
            DWORD newestPacketNumber = 0;
            for (DWORD userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
                if (connectedUsers[userIndex] &&
                    (connectedUserIndex == XUSER_MAX_COUNT ||
                        xinputStates[userIndex].dwPacketNumber > newestPacketNumber)) {
                    connectedUserIndex = userIndex;
                    newestPacketNumber = xinputStates[userIndex].dwPacketNumber;
                }
            }
        }

        if (connectedUserIndex == XUSER_MAX_COUNT) {
            // 未接続、または切断された場合は状態をクリア
            activeUserIndex_ = XUSER_MAX_COUNT;
            state_.isConnected = false;
            state_.buttons = 0;
            state_.leftStick = {};
            state_.rightStick = {};
            state_.leftTrigger = 0.0f;
            state_.rightTrigger = 0.0f;
            return;
        }

        if (connectedUserIndex != activeUserIndex_) {
            // A newly selected controller must not inherit edge states from
            // the controller that was previously active.
            state_.preButtons = 0;
            state_.preRightTrigger = 0.0f;
        }
        activeUserIndex_ = connectedUserIndex;
        state_.isConnected = true;

        const XINPUT_STATE& xstate = xinputStates[connectedUserIndex];
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

    bool GamePad::PushRightTrigger(int /*index*/) const {
        return state_.rightTrigger > 0.0f;
    }

    bool GamePad::TriggerRightTrigger(int /*index*/) const {
        return state_.rightTrigger > 0.0f &&
            state_.preRightTrigger <= 0.0f;
    }

    bool GamePad::ExitRightTrigger(int /*index*/) const {
        return state_.rightTrigger <= 0.0f && state_.preRightTrigger > 0.0f;
    }

    void GamePad::SetVibration(float leftMotor, float rightMotor, int /*index*/) {
        if (activeUserIndex_ >= XUSER_MAX_COUNT) {
            return;
        }
        leftMotor = std::clamp(leftMotor, 0.0f, 1.0f);
        rightMotor = std::clamp(rightMotor, 0.0f, 1.0f);

        XINPUT_VIBRATION vibration{};
        vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
        vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);
        XInputSetState(activeUserIndex_, &vibration);
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
