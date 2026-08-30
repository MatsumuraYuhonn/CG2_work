#pragma once

#include <d3d12.h>

#include "MTEngine/Engine/Graphics/ConstantBuffer.h"
#include "MTEngine/Engine/Graphics/Model.h"
#include "MTEngine/Engine/Graphics/Sprite.h"
#include "MTEngine/Engine/Graphics/TextureManager.h"
#include "MTEngine/Engine/Math/Matrix.h"
#include "MTEngine/Engine/Math/Vector.h"

#include <cstdint>
#include <array>
#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace MTEngine {

    class Input;

    // CSV で指定された 1 マス分のマップチップ情報です。
    struct TileMapCell {
        int32_t tileId = -1;
        int32_t column = 0;
        int32_t row = 0;
        Vector3 worldPosition{};
    };

    // 整数 CSV を読み込み、マップチップを配置するためのステージデータを保持します。
    //
    // CSV 例 ( 0 は空マス ):
    //  1,1,2,2
    //  1,0,0,2
    //  3,3,3,3
    class TileMap {
    public:
        static constexpr int32_t kEmptyTileId = 0;

        // CSV を読み込みます。失敗時は false を返し、GetLastError() に理由を設定します。
        bool LoadFromCsv(const std::string& filePath);

        void Clear();

        int32_t GetWidth() const { return width_; }
        int32_t GetHeight() const { return height_; }
        bool IsEmpty() const { return tiles_.empty(); }
        const std::string& GetLastError() const { return lastError_; }

        // 範囲外の場合は kEmptyTileId を返します。
        int32_t GetTileId(int32_t column, int32_t row) const;

        // 空マスを除いた配置情報を作成します。
        // column は +X、row は -Z 方向に並びます。
        std::vector<TileMapCell> CreateCells(float tileSize, const Vector3& origin = {}) const;

    private:
        int32_t width_ = 0;
        int32_t height_ = 0;
        std::vector<int32_t> tiles_;
        std::string lastError_;
    };

    // Loads the cube resource once and draws one cube for every non-empty CSV cell.
    // Owns the gameplay objects in this scene: the tile map and player.
    class GameScene {
    public:
        bool Initialize(
            Microsoft::WRL::ComPtr<ID3D12Device> device,
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
            DescriptorHeapManager* srvHeapManager,
            const std::string& csvFilePath);

        void Draw(
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
            const Matrix4x4& viewMatrix,
            const Matrix4x4& projectionMatrix,
            float elapsedGameTimeSeconds,
            bool showGameTimer);
        void DrawSkyDome(
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
            const Matrix4x4& viewMatrix,
            const Matrix4x4& projectionMatrix);
        void DrawTitleScreen(
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
        void DrawTitleOverlay(
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
        void DrawRankingScreen(
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
        void DrawResultScreen(
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
            float elapsedGameTimeSeconds,
            bool isClear,
            bool showGoNightmareTip);
        void SetClearRanking(const std::vector<double>& bestTimes) {
            clearRankingCount_ = bestTimes.size() < clearRankingTimes_.size()
                ? bestTimes.size()
                : clearRankingTimes_.size();
            for (size_t index = 0; index < clearRankingCount_; ++index) {
                clearRankingTimes_[index] = static_cast<float>(bestTimes[index]);
            }
        }
        bool IsOrientationMarkerVisible() const {
            return isOrientationMarkerVisible_;
        }
        void SetOrientationMarkerVisible(bool isVisible) {
            isOrientationMarkerVisible_ = isVisible;
        }
        bool IsNightmareMode() const { return isNightmareMode_; }
        void SetNightmareMode(bool isEnabled) {
            isNightmareMode_ = isEnabled;
        }

        // Advances player movement and physics against non-empty map tiles.
        void UpdateSkyDome();
        void UpdatePlayer(const Input* input);
        // Restores gameplay state when a new game starts from the title scene.
        void Reset();
        // Sets up stage 0 with only the player for the tutorial scene.
        void PrepareTutorial();
        bool IsTutorialComplete() const;
        void SetTutorialSkipHoldProgress(float progress) {
            tutorialSkipHoldProgress_ = progress < 0.0f
                ? 0.0f
                : (progress > 1.0f ? 1.0f : progress);
        }

        bool AreAllEnemiesDefeated() const;
        bool IsEnemyDefeatAnimationActive() const {
            return isEnemyDefeatAnimationActive_ ||
                (bossPhase_ == 1 && isEnemyDefeatAnimationFinished_);
        }
        float GetEnemyDefeatCameraPullBackProgress() const;
        Vector3 GetEnemyDefeatCameraTarget() const;
        bool IsPhase1IntroActive() const { return isPhase1IntroActive_; }
        float GetPhase1IntroCameraPullBackProgress() const;
        Vector3 GetPhase1IntroCameraTarget() const {
            if (enemyPositions_.empty()) {
                return { 0.0f, 0.0f, 2.134f };
            }
            return {
                enemyPositions_[0].x,
                enemyPositions_[0].y,
                2.134f,
            };
        }
        bool IsPhase2IntroActive() const { return isPhase2IntroActive_; }
        float GetPhase2IntroCameraPullBackProgress() const;
        Vector3 GetPhase2IntroCameraTarget() const {
            if (enemyPositions_.empty()) {
                return { 0.0f, 0.0f, 2.134f };
            }
            return {
                enemyPositions_[0].x,
                enemyPositions_[0].y,
                2.134f,
            };
        }
        // Returns true when another boss phase was started.
        bool TryAdvanceBossPhase();
        bool IsPlayerDefeated() const;
        int32_t GetBossPhase() const { return bossPhase_; }
        bool SetBossPhase(int32_t phase);
        void SpawnPhase2AttackEffects();
        void SpawnPhase2RainAttack();
        void SpawnPhase2FastSpinAttack();
        void SpawnPhase2SideSweepAttack();

        void SetSelectedCellIndex(int32_t selectedCellIndex) { selectedCellIndex_ = selectedCellIndex; }

        const TileMap& GetTileMap() const { return tileMap_; }
        const std::vector<TileMapCell>& GetCells() const { return cells_; }
        const std::vector<Vector3>& GetEnemyPositions() const { return enemyPositions_; }
        float GetEnemyHealth(size_t enemyIndex) const {
            return enemyIndex < enemyHealths_.size() ? enemyHealths_[enemyIndex] : 0.0f;
        }
        float GetEnemyMaxHealth() const { return bossPhase_ == 2 ? 300.0f : 100.0f; }
        const Vector3& GetPlayerPosition() const { return playerPosition_; }
        float GetPlayerHealth() const { return playerHealth_; }
        float GetPlayerMaxHealth() const {
            return isNightmareMode_ ? 10.0f : 100.0f;
        }
        float GetPlayerVerticalVelocity() const { return playerVerticalVelocity_; }
        float GetPlayerHorizontalVelocity() const { return playerHorizontalVelocity_; }
        float GetPlayerMoveAcceleration() const { return playerMoveAcceleration_; }
        float GetPlayerVelocityDamping() const { return playerVelocityDamping_; }
        float GetPlayerMaxMoveSpeed() const { return playerMaxMoveSpeed_; }
        float GetPlayerJumpSpeed() const { return playerJumpSpeed_; }
        int32_t GetRemainingProjectileCount() const;
        bool IsPlayerCharging() const { return isPlayerCharging_; }
        bool ShouldPlayPlayerChargeSound() const {
            constexpr float kChargeSecondsPerAmmo = 0.25f;
            return isPlayerCharging_ &&
                playerChargeTime_ >= kChargeSecondsPerAmmo;
        }
        bool IsPlayerChargeComplete() const {
            constexpr float kChargeSecondsPerAmmo = 0.25f;
            const int32_t remainingAmmo = GetRemainingProjectileCount();
            return remainingAmmo > 0 &&
                playerChargeTime_ >= kChargeSecondsPerAmmo *
                    static_cast<float>(remainingAmmo - 1);
        }
        int32_t ConsumeFiredProjectileAmmo() {
            const int32_t firedAmmo = firedProjectileAmmo_;
            firedProjectileAmmo_ = 0;
            return firedAmmo;
        }
        bool ConsumeTeleportSoundRequest() {
            const bool isRequested = teleportSoundRequested_;
            teleportSoundRequested_ = false;
            return isRequested;
        }
        bool ConsumeDashSoundRequest() {
            const bool isRequested = dashSoundRequested_;
            dashSoundRequested_ = false;
            return isRequested;
        }
        bool ConsumeAlertSoundRequest() {
            const bool isRequested = alertSoundRequested_;
            alertSoundRequested_ = false;
            return isRequested;
        }
        bool ConsumeFallSoundRequest() {
            const bool isRequested = fallSoundRequested_;
            fallSoundRequested_ = false;
            return isRequested;
        }
        bool ConsumeDamageSoundRequest() {
            const bool isRequested = damageSoundRequested_;
            damageSoundRequested_ = false;
            return isRequested;
        }
        bool ConsumeEnemyDamageSoundRequest() {
            const bool isRequested = enemyDamageSoundRequested_;
            enemyDamageSoundRequested_ = false;
            return isRequested;
        }
        bool IsPlayerGrounded() const { return isPlayerGrounded_; }
        void SetPlayerPosition(const Vector3& position) {
            playerPosition_ = position;
            playerHorizontalVelocity_ = 0.0f;
            playerVerticalVelocity_ = 0.0f;
            isPlayerGrounded_ = false;
        }
        void SetPlayerHealth(float health) {
            const float maxHealth = GetPlayerMaxHealth();
            playerHealth_ = health < 0.0f
                ? 0.0f
                : (health > maxHealth ? maxHealth : health);
        }
        void SetPlayerMoveAcceleration(float acceleration) { playerMoveAcceleration_ = acceleration < 0.0f ? 0.0f : acceleration; }
        void SetPlayerVelocityDamping(float damping) { playerVelocityDamping_ = damping < 0.0f ? 0.0f : damping; }
        void SetPlayerJumpSpeed(float speed) { playerJumpSpeed_ = speed < 0.0f ? 0.0f : speed; }
        void SetEnemyPosition(size_t enemyIndex, const Vector3& position) {
            if (enemyIndex < enemyPositions_.size()) {
                enemyPositions_[enemyIndex] = position;
                enemyVerticalVelocities_[enemyIndex] = 0.0f;
            }
        }
        void SetEnemyHealth(size_t enemyIndex, float health) {
            if (enemyIndex < enemyHealths_.size()) {
                const float maxHealth = GetEnemyMaxHealth();
                enemyHealths_[enemyIndex] =
                    health < 0.0f ? 0.0f : (health > maxHealth ? maxHealth : health);
            }
        }
        void SetPlayerMaxMoveSpeed(float maxMoveSpeed) {
            playerMaxMoveSpeed_ = maxMoveSpeed < 0.0f ? 0.0f : maxMoveSpeed;
            if (playerHorizontalVelocity_ > playerMaxMoveSpeed_) {
                playerHorizontalVelocity_ = playerMaxMoveSpeed_;
            }
            else if (playerHorizontalVelocity_ < -playerMaxMoveSpeed_) {
                playerHorizontalVelocity_ = -playerMaxMoveSpeed_;
            }
        }

    private:
        // A small state machine for the initial patrol behaviour.  Keeping this
        // per enemy makes it possible to add chase/attack states later without
        // changing the position and physics ownership in GameScene.
        enum class EnemyBehavior {
            Stop,
            Move,
            JumpPrepare,
            AttackPrepare,
            Attack,
        };

        enum class Phase2AttackPattern {
            EightWay,
            Rain,
            FastSpin,
            SideSweep,
        };

        enum class Phase2TeleportState {
            Idle,
            Disappearing,
            Appearing,
        };

        enum class TutorialStep {
            Move,
            Jump,
            Attack,
            ChargeAttack,
            Complete,
        };

        float GetRandomEnemyBehaviorDuration(EnemyBehavior behavior);
        float GetRandomEnemyMoveSpeed();
        float GetRandomEnemySpeedVariationDuration();
        float GetAttackIntervalScale() const {
            return isNightmareMode_ ? 0.8f : 1.0f;
        }
        float GetPhase2AttackTelegraphDuration() const;
        Vector3 GetRandomEnemyFloatDirection();
        float GetRandomPhase2TeleportInterval();
        bool LoadStageForPhase(int32_t phase);
        void SpawnPhase2AttackPattern(
            Phase2AttackPattern pattern,
            bool isCloneAttack = false);
        void StartEnemyDefeatAnimation();

        struct Projectile {
            Vector3 position{};
            Vector3 direction{};
            float remainingTime = 0.0f;
            float damage = 1.0f;
            float scale = 0.25f;
            int32_t ammoCost = 0;
            bool isActive = false;
        };

        static constexpr size_t kMaxProjectiles = 10;
        static constexpr size_t kPhase2AttackEffectCount = 8;
        static constexpr size_t kPhase2CircleTelegraphSegmentCount = 32;
        static constexpr int32_t kBossPhaseCount = 2;

        TileMap tileMap_;
        std::vector<TileMapCell> cells_;
        Microsoft::WRL::ComPtr<ID3D12Device> device_;
        Model skyDomeModel_;
        Model cubeModel_;
        Model bulletModel_;
        Model playerModel_;
        Model enemyModel_;
        Model enemyPhase2BodyModel_;
        Model enemyPhase2Body2Model_;
        Model enemyPhase2WeaponModel_;
        Model enemyAttackEffectModel_;
        Sprite uiSprite_;
        Sprite enemyHpSprite_;
        Sprite playerHpSprite_;
        Sprite gameClearSprite_;
        Sprite gameOverSprite_;
        Sprite goNightmareSprite_;
        Sprite goNightmareStartButtonSprite_;
        Sprite titleControllerSprite_;
        Sprite titleLogoSprite_;
        Sprite titleNightmareSprite_;
        Sprite titleGameStartSprite_;
        Sprite titleToRankingSprite_;
        Sprite rankingBackgroundSprite_;
        Sprite nightmareRankingBackgroundSprite_;
        std::array<Sprite, kMaxProjectiles> bulletCountSprites_;
        std::array<Sprite, 4> timerDigitSprites_;
        Sprite timerColonSprite_;
        std::array<Sprite, 6> resultTimeDigitSprites_;
        std::array<Sprite, 2> resultTimeColonSprites_;
        std::array<std::array<Sprite, 6>, 3> resultRankingDigitSprites_;
        std::array<std::array<Sprite, 2>, 3> resultRankingColonSprites_;
        std::array<Sprite, 5> tutorialButtonSprites_;
        Sprite tutorialStartButtonSprite_;
        Sprite tutorialStartArrowSprite_;
        Sprite tutorialSkipGaugeSprite_;
        std::array<D3D12_GPU_DESCRIPTOR_HANDLE, 10> timerDigitTextureHandles_{};
        std::array<float, 3> clearRankingTimes_{};
        size_t clearRankingCount_ = 0;
        TextureManager textureManager_;
        MaterialConstantBuffer skyDomeMaterialConstantBuffer_;
        TransformationMatrixConstantBuffer skyDomeTransformConstantBuffer_;
        MaterialConstantBuffer materialConstantBuffer_;
        MaterialConstantBuffer playerMaterialConstantBuffer_;
        MaterialConstantBuffer enemyMaterialConstantBuffer_;
        MaterialConstantBuffer phase2CloneMaterialConstantBuffer_;
        MaterialConstantBuffer enemyAttackEffectMaterialConstantBuffer_;
        MaterialConstantBuffer phase2CloneAttackEffectMaterialConstantBuffer_;
        MaterialConstantBuffer projectileMaterialConstantBuffer_;
        std::vector<std::unique_ptr<TransformationMatrixConstantBuffer>> transformConstantBuffers_;
        TransformationMatrixConstantBuffer playerTransformConstantBuffer_;
        std::vector<std::unique_ptr<TransformationMatrixConstantBuffer>> enemyTransformConstantBuffers_;
        std::vector<std::unique_ptr<TransformationMatrixConstantBuffer>> enemyPhase2WeaponTransformConstantBuffers_;
        TransformationMatrixConstantBuffer phase2CloneTransformConstantBuffer_;
        std::array<TransformationMatrixConstantBuffer, kPhase2CircleTelegraphSegmentCount> enemyAttackEffectTransformConstantBuffers_;
        std::array<TransformationMatrixConstantBuffer, kPhase2CircleTelegraphSegmentCount> phase2CloneAttackEffectTransformConstantBuffers_;
        std::vector<std::unique_ptr<TransformationMatrixConstantBuffer>> projectileTransformConstantBuffers_;
        TransformationMatrixConstantBuffer chargingProjectileTransformConstantBuffer_;
        MaterialConstantBuffer selectionMaterialConstantBuffer_;
        std::array<MaterialConstantBuffer, 3> axisMaterialConstantBuffers_;
        std::array<TransformationMatrixConstantBuffer, 3> axisTransformConstantBuffers_;
        bool isOrientationMarkerVisible_ = false;
        DirectionalLightConstantBuffer lightConstantBuffer_;
        Vector3 playerPosition_{};
        std::vector<Vector3> enemyPositions_;
        std::vector<float> enemyVerticalVelocities_;
        std::vector<float> enemyHealths_;
        std::vector<EnemyBehavior> enemyBehaviors_;
        std::vector<float> enemyBehaviorTimers_;
        std::vector<float> enemyMoveDirections_;
        std::vector<float> enemyMoveSpeeds_;
        std::vector<float> enemyTargetMoveSpeeds_;
        std::vector<float> enemySpeedVariationTimers_;
        std::vector<Vector3> enemyFloatDirections_;
        std::vector<bool> enemyGrounded_;
        std::vector<float> enemyAttackCooldowns_;
        std::vector<bool> enemyAttackHitPlayers_;
        std::vector<bool> enemyRepeatDashUsed_;
        std::vector<Vector3> enemyAttackDirections_;
        std::vector<Vector3> enemyAttackStartPositions_;
        std::array<Vector3, kPhase2AttackEffectCount> phase2AttackEffectPositions_{};
        std::array<Vector3, kPhase2AttackEffectCount> phase2AttackEffectDirections_{};
        float phase2AttackEffectRotation_ = 0.0f;
        float phase2AttackEffectRotationDirection_ = 1.0f;
        float phase2AttackEffectElapsedTime_ = 0.0f;
        float phase2AttackDamageCooldown_ = 0.0f;
        float phase2NextAttackTimer_ = 2.5f;
        float phase2TeleportTimer_ = 10.0f;
        size_t phase2TeleportPositionIndex_ = 0;
        size_t phase2TeleportTargetPositionIndex_ = 0;
        float phase2TeleportEffectTimer_ = 0.0f;
        Phase2TeleportState phase2TeleportState_ = Phase2TeleportState::Idle;
        size_t phase2AttackEffectActiveCount_ = kPhase2AttackEffectCount;
        Phase2AttackPattern phase2AttackPattern_ = Phase2AttackPattern::EightWay;
        size_t phase2SideSweepWaveIndex_ = 0;
        float phase2SideSweepFirstDirection_ = 1.0f;
        bool arePhase2AttackEffectsActive_ = false;
        bool isPhase2CloneActive_ = false;
        bool isPhase2ClonePending_ = false;
        Phase2AttackPattern pendingPhase2CloneAttackPattern_ =
            Phase2AttackPattern::EightWay;
        bool isPhase2BossHiddenForClone_ = false;
        bool shouldSpawnPhase2CloneOnNextTeleport_ = true;
        std::mt19937 enemyRandomEngine_{ std::random_device{}() };
        std::vector<Projectile> projectiles_;
        float playerHealth_ = 100.0f;
        bool isNightmareMode_ = false;
        int32_t bossPhase_ = 1;
        float enemyContactDamageCooldown_ = 0.0f;
        float playerInvincibilityTimer_ = 0.0f;
        float playerHitFlashTimer_ = 0.0f;
        float enemyHitFlashTimer_ = 0.0f;
        float playerRotationY_ = 0.0f;
        float playerTargetRotationY_ = 0.0f;
        float playerDeathRotationX_ = 0.0f;
        float playerDeathAnimationTimer_ = 0.0f;
        bool isPlayerDeathAnimationActive_ = false;
        bool isPlayerDeathAnimationFinished_ = false;
        float playerChargeTime_ = 0.0f;
        bool isPlayerCharging_ = false;
        int32_t firedProjectileAmmo_ = 0;
        bool teleportSoundRequested_ = false;
        bool dashSoundRequested_ = false;
        bool alertSoundRequested_ = false;
        bool fallSoundRequested_ = false;
        bool damageSoundRequested_ = false;
        bool enemyDamageSoundRequested_ = false;
        bool isEnemyDefeatAnimationActive_ = false;
        bool isEnemyDefeatAnimationFinished_ = false;
        float enemyDefeatAnimationTimer_ = 0.0f;
        bool isPhase1IntroActive_ = false;
        float phase1IntroElapsedTime_ = 0.0f;
        bool isPhase2IntroActive_ = false;
        float phase2IntroElapsedTime_ = 0.0f;
        Vector3 phase2IntroStartPosition_{};
        bool isTutorialMode_ = false;
        TutorialStep tutorialStep_ = TutorialStep::Move;
        float tutorialStartPlayerX_ = 0.0f;
        float tutorialIconAnimationTime_ = 0.0f;
        float tutorialAttackHintTimer_ = 0.0f;
        bool hasTutorialJumpStarted_ = false;
        bool suppressPlayerInputForOneFrame_ = false;
        float tutorialSkipHoldProgress_ = 0.0f;
        float skyDomeRotationY_ = 0.0f;
        float titleAnimationTime_ = 0.0f;
        float playerHorizontalVelocity_ = 0.0f;
        float playerVerticalVelocity_ = 0.0f;
        float playerMoveAcceleration_ = 50.0f;
        float playerVelocityDamping_ = 10.0f;
        float playerMaxMoveSpeed_ = 15.0f;
        float playerJumpSpeed_ = 22.0f;
        bool isPlayerGrounded_ = false;
        std::chrono::steady_clock::time_point previousSkyDomeUpdateTime_{};
        std::chrono::steady_clock::time_point previousPlayerUpdateTime_{};
        int32_t selectedCellIndex_ = -1;
    };

}
