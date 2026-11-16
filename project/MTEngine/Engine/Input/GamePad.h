#pragma once
#include <windows.h>
#include <Xinput.h>
#include <cstdint>

#pragma comment(lib, "xinput.lib")

// ゲームパッド入力管理クラス（XInput使用、1台のみ対応）
namespace MTEngine {

    // ゲームパッドのボタン種別（XINPUT_GAMEPADのビットマスクに対応）
    enum class GamePadButton : WORD {
        Up = XINPUT_GAMEPAD_DPAD_UP,
        Down = XINPUT_GAMEPAD_DPAD_DOWN,
        Left = XINPUT_GAMEPAD_DPAD_LEFT,
        Right = XINPUT_GAMEPAD_DPAD_RIGHT,
        Start = XINPUT_GAMEPAD_START,
        Back = XINPUT_GAMEPAD_BACK,
        LThumb = XINPUT_GAMEPAD_LEFT_THUMB,
        RThumb = XINPUT_GAMEPAD_RIGHT_THUMB,
        LShoulder = XINPUT_GAMEPAD_LEFT_SHOULDER,
        RShoulder = XINPUT_GAMEPAD_RIGHT_SHOULDER,
        A = XINPUT_GAMEPAD_A,
        B = XINPUT_GAMEPAD_B,
        X = XINPUT_GAMEPAD_X,
        Y = XINPUT_GAMEPAD_Y,
    };

    // スティックの入力値（-1.0f 〜 1.0f に正規化済み、デッドゾーン適用後）
    struct StickState {
        float x = 0.0f;
        float y = 0.0f;
    };

    // 1台分のゲームパッドの状態
    struct GamePadState {
        bool isConnected = false;
        WORD buttons = 0;          // 現在のフレームのボタン状態
        WORD preButtons = 0;       // 前のフレームのボタン状態
        StickState leftStick{};
        StickState rightStick{};
        float leftTrigger = 0.0f;  // 0.0f 〜 1.0f
        float rightTrigger = 0.0f; // 0.0f 〜 1.0f
        float preRightTrigger = 0.0f;
    };

    class GamePad {
    public:
        GamePad() = default;
        ~GamePad() = default;

        GamePad(const GamePad&) = delete;
        GamePad& operator=(const GamePad&) = delete;

        // 初期化（接続確認のみ、DirectInputと違い明示的な初期化は不要）
        void Initialize();
        // ゲームパッド入力状態の更新
        void Update();

        // コントローラーが接続されているか
        bool IsConnected(int index = 0) const;

        // ボタンが押されているか
        bool PushButton(GamePadButton button, int index = 0) const;
        // ボタンがトリガーされた（押された瞬間）か
        bool TriggerButton(GamePadButton button, int index = 0) const;
        // ボタンが離された瞬間か
        bool ExitButton(GamePadButton button, int index = 0) const;

        // 左スティックの入力値取得（デッドゾーン適用後、-1.0f〜1.0f）
        const StickState& GetLeftStick(int index = 0) const;
        // 右スティックの入力値取得（デッドゾーン適用後、-1.0f〜1.0f）
        const StickState& GetRightStick(int index = 0) const;

        // 左トリガーの入力値取得（0.0f〜1.0f）
        float GetLeftTrigger(int index = 0) const;
        // 右トリガーの入力値取得（0.0f〜1.0f）
        float GetRightTrigger(int index = 0) const;
        // 右トリガーが押されているか
        bool PushRightTrigger(int index = 0) const;
        // 右トリガーが押された瞬間か
        bool TriggerRightTrigger(int index = 0) const;
        // 右トリガーが離された瞬間か
        bool ExitRightTrigger(int index = 0) const;

        // 振動（バイブレーション）の設定。0.0f〜1.0fで指定
        void SetVibration(float leftMotor, float rightMotor, int index = 0);
        // 振動を止める
        void StopVibration(int index = 0);

    private:
        static float NormalizeStickAxis(SHORT value, SHORT deadZone);
        static float NormalizeTrigger(BYTE value, BYTE deadZone);

        GamePadState state_{};
        DWORD activeUserIndex_ = XUSER_MAX_COUNT;
    };

}
