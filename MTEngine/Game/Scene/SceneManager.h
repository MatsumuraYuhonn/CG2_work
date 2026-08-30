#pragma once

#include "MTEngine/Engine/Input/Input.h"
#include "MTEngine/Game/Scene/GameScene.h"
#include "MTEngine/Game/Save/ClearTimeRecorder.h"

#include <chrono>

namespace MTEngine {

    enum class SceneType {
        Title,
        Tutorial,
        Game,
        Clear,
        Miss,
    };

    // TitleScene, ClearScene, and MissScene currently own only their transition
    // rules. GameScene continues to own all gameplay objects and rendering data.
    class TitleScene {
    public:
        bool IsStartRequested(const Input* input) const {
            return input && (input->TriggerKey(DIK_RETURN) ||
                (input->GetGamePad() &&
                    input->GetGamePad()->TriggerButton(GamePadButton::A)));
        }

        bool IsRankingToggleRequested(const Input* input) const {
            return input && input->GetGamePad() &&
                input->GetGamePad()->TriggerRightTrigger();
        }

        bool IsRankingOpen() const { return isRankingOpen_; }
        void ToggleRanking() { isRankingOpen_ = !isRankingOpen_; }
        void CloseRanking() { isRankingOpen_ = false; }

        bool IsNightmareToggleRequested(const Input* input) {
            constexpr float kNightmareHoldDurationSeconds = 3.0f;
            const bool isStartHeld = input && input->GetGamePad() &&
                input->GetGamePad()->PushButton(GamePadButton::Start);
            if (!isStartHeld) {
                isNightmareHoldActive_ = false;
                hasNightmareToggleTriggered_ = false;
                nightmareHoldStartTime_ = {};
                return false;
            }
            if (!isNightmareHoldActive_) {
                isNightmareHoldActive_ = true;
                nightmareHoldStartTime_ = std::chrono::steady_clock::now();
                return false;
            }
            if (hasNightmareToggleTriggered_) {
                return false;
            }

            const float heldSeconds = std::chrono::duration<float>(
                std::chrono::steady_clock::now() -
                    nightmareHoldStartTime_).count();
            if (heldSeconds < kNightmareHoldDurationSeconds) {
                return false;
            }
            hasNightmareToggleTriggered_ = true;
            return true;
        }

    private:
        bool isNightmareHoldActive_ = false;
        bool hasNightmareToggleTriggered_ = false;
        bool isRankingOpen_ = false;
        std::chrono::steady_clock::time_point nightmareHoldStartTime_{};
    };

    class TutorialScene {
    public:
        bool IsCompleteRequested(const Input* input) const {
            return input && (input->TriggerKey(DIK_RETURN) ||
                (input->GetGamePad() && input->GetGamePad()->TriggerButton(GamePadButton::Start)));
        }

        bool IsSkipRequested(const Input* input) {
            constexpr float kSkipHoldDurationSeconds = 1.0f;
            const bool isSkipHeld = input &&
                (input->PushKey(DIK_RETURN) ||
                    (input->GetGamePad() &&
                        input->GetGamePad()->PushButton(GamePadButton::Start)));
            if (!isSkipHeld) {
                ResetSkipHold();
                return false;
            }

            const auto now = std::chrono::steady_clock::now();
            if (!isSkipHoldActive_) {
                isSkipHoldActive_ = true;
                skipHoldStartTime_ = now;
                skipHoldProgress_ = 0.0f;
                return false;
            }

            const float heldSeconds = std::chrono::duration<float>(
                now - skipHoldStartTime_).count();
            skipHoldProgress_ = heldSeconds >= kSkipHoldDurationSeconds
                ? 1.0f
                : heldSeconds / kSkipHoldDurationSeconds;
            return skipHoldProgress_ >= 1.0f;
        }

        float GetSkipHoldProgress() const { return skipHoldProgress_; }

        void ResetSkipHold() {
            isSkipHoldActive_ = false;
            skipHoldStartTime_ = {};
            skipHoldProgress_ = 0.0f;
        }

    private:
        bool isSkipHoldActive_ = false;
        std::chrono::steady_clock::time_point skipHoldStartTime_{};
        float skipHoldProgress_ = 0.0f;
    };

    class ClearScene {
    public:
        bool IsReturnRequested(const Input* input) const {
            return input && (input->TriggerKey(DIK_RETURN) ||
                (input->GetGamePad() && input->GetGamePad()->TriggerButton(GamePadButton::Start)));
        }
        bool IsGoNightmareTipOpen() const {
            return isGoNightmareTipOpen_;
        }
        void OpenGoNightmareTip() {
            isGoNightmareTipOpen_ = true;
        }
        void Reset() {
            isGoNightmareTipOpen_ = false;
        }

    private:
        bool isGoNightmareTipOpen_ = false;
    };

    class MissScene {
    public:
        bool IsReturnRequested(const Input* input) const {
            return input && (input->TriggerKey(DIK_RETURN) ||
                (input->GetGamePad() && input->GetGamePad()->TriggerButton(GamePadButton::Start)));
        }
    };

    // Owns the current scene and evaluates the rules for changing between scenes.
    class SceneManager {
    public:
        void Update(const Input* input, GameScene& gameScene) {
            switch (currentScene_) {
            case SceneType::Title:
                if (titleScene_.IsRankingToggleRequested(input)) {
                    titleScene_.ToggleRanking();
                    if (titleScene_.IsRankingOpen()) {
                        gameScene.SetClearRanking(
                            ClearTimeRecorder::GetBestTimes(
                                gameScene.IsNightmareMode()));
                    }
                    break;
                }
                if (titleScene_.IsRankingOpen()) {
                    break;
                }
                if (titleScene_.IsNightmareToggleRequested(input)) {
                    gameScene.SetNightmareMode(
                        !gameScene.IsNightmareMode());
                }
                if (titleScene_.IsStartRequested(input)) {
                    gameScene.PrepareTutorial();
                    tutorialScene_.ResetSkipHold();
                    elapsedGameTimeSeconds_ = 0.0f;
                    currentScene_ = SceneType::Tutorial;
                }
                break;

            case SceneType::Tutorial:
                if (tutorialScene_.IsSkipRequested(input) ||
                    (gameScene.IsTutorialComplete() &&
                        tutorialScene_.IsCompleteRequested(input))) {
                    tutorialScene_.ResetSkipHold();
                    gameScene.Reset();
                    currentScene_ = SceneType::Game;
                }
                break;

            case SceneType::Game:
                if (!isGameTimerRunning_) {
                    if (gameScene.IsPhase1IntroActive()) {
                        break;
                    }
                    StartGameTimer();
                }
                if (gameScene.IsPlayerDefeated()) {
                    StopGameTimer();
                    currentScene_ = SceneType::Miss;
                }
                else if (gameScene.GetPlayerHealth() <= 0.0f) {
                    // Wait for the player's death animation before changing scene.
                    break;
                }
                else if (gameScene.AreAllEnemiesDefeated()) {
                    if (!gameScene.TryAdvanceBossPhase()) {
                        StopGameTimer();
                        const bool isNightmareMode =
                            gameScene.IsNightmareMode();
                        ClearTimeRecorder::Save(
                            elapsedGameTimeSeconds_,
                            isNightmareMode);
                        gameScene.SetClearRanking(
                            ClearTimeRecorder::GetBestTimes(
                                isNightmareMode));
                        clearScene_.Reset();
                        currentScene_ = SceneType::Clear;
                    }
                }
                break;

            case SceneType::Clear:
                if (clearScene_.IsReturnRequested(input)) {
                    if (!gameScene.IsNightmareMode() &&
                        !clearScene_.IsGoNightmareTipOpen()) {
                        clearScene_.OpenGoNightmareTip();
                    }
                    else {
                        titleScene_.CloseRanking();
                        currentScene_ = SceneType::Title;
                    }
                }
                break;

            case SceneType::Miss:
                if (missScene_.IsReturnRequested(input)) {
                    titleScene_.CloseRanking();
                    currentScene_ = SceneType::Title;
                }
                break;
            }
        }

        SceneType GetCurrentScene() const { return currentScene_; }
        bool IsGameScene() const { return currentScene_ == SceneType::Game; }
        bool IsTitleRankingOpen() const {
            return currentScene_ == SceneType::Title &&
                titleScene_.IsRankingOpen();
        }
        bool IsGoNightmareTipOpen() const {
            return currentScene_ == SceneType::Clear &&
                clearScene_.IsGoNightmareTipOpen();
        }
        bool IsPlayableScene() const {
            return currentScene_ == SceneType::Tutorial || currentScene_ == SceneType::Game;
        }
        float GetTutorialSkipHoldProgress() const {
            return currentScene_ == SceneType::Tutorial
                ? tutorialScene_.GetSkipHoldProgress()
                : 0.0f;
        }

        void ChangeScene(SceneType scene, GameScene& gameScene) {
            if (isGameTimerRunning_ && scene != SceneType::Game) {
                StopGameTimer();
            }
            if (scene == SceneType::Tutorial) {
                gameScene.PrepareTutorial();
                tutorialScene_.ResetSkipHold();
                elapsedGameTimeSeconds_ = 0.0f;
            }
            else if (scene == SceneType::Title) {
                titleScene_.CloseRanking();
            }
            else if (scene == SceneType::Clear) {
                clearScene_.Reset();
            }
            else if (scene == SceneType::Game) {
                tutorialScene_.ResetSkipHold();
                gameScene.Reset();
            }
            currentScene_ = scene;
        }

        float GetElapsedGameTimeSeconds() const {
            if (!isGameTimerRunning_) {
                return elapsedGameTimeSeconds_;
            }
            return std::chrono::duration<float>(
                std::chrono::steady_clock::now() - gameStartTime_).count();
        }

        const char* GetCurrentSceneName() const {
            switch (currentScene_) {
            case SceneType::Title: return "Title";
            case SceneType::Tutorial: return "Tutorial";
            case SceneType::Game:  return "Game";
            case SceneType::Clear: return "Clear";
            case SceneType::Miss:  return "Miss";
            }
            return "Unknown";
        }

        const char* GetTransitionCondition() const {
            switch (currentScene_) {
            case SceneType::Title: return "GamePad A: open tutorial | Hold Start for 3 seconds: toggle Nightmare";
            case SceneType::Tutorial: return "Complete training and press Start, or hold Start for 1 second: skip";
            case SceneType::Game:  return "Boss phase 2 defeated: clear | HP 0 or fall: miss";
            case SceneType::Clear: return "Enter / GamePad Start: return to title";
            case SceneType::Miss:  return "Enter / GamePad Start: return to title";
            }
            return "";
        }

    private:
        void StartGameTimer() {
            gameStartTime_ = std::chrono::steady_clock::now();
            elapsedGameTimeSeconds_ = 0.0f;
            isGameTimerRunning_ = true;
        }

        void StopGameTimer() {
            if (!isGameTimerRunning_) {
                return;
            }
            elapsedGameTimeSeconds_ = GetElapsedGameTimeSeconds();
            isGameTimerRunning_ = false;
        }

        SceneType currentScene_ = SceneType::Title;
        TitleScene titleScene_;
        TutorialScene tutorialScene_;
        ClearScene clearScene_;
        MissScene missScene_;
        std::chrono::steady_clock::time_point gameStartTime_{};
        float elapsedGameTimeSeconds_ = 0.0f;
        bool isGameTimerRunning_ = false;
    };

}
