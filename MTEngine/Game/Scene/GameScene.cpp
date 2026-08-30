#include "GameScene.h"
#include "MTEngine/Engine/Base/Window.h"
#include "MTEngine/Engine/Input/Input.h"

#include <charconv>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <fstream>
#include <numbers>
#include <sstream>
#include <utility>

namespace MTEngine {

    namespace {

        constexpr float kPhase2AttackEffectModelHalfLength = 61.09984f;
        constexpr float kPhase2AttackEffectScale = 0.7f;
        constexpr float kPhase2AttackEffectWidthScale = 1.00f;
        constexpr float kPhase2AttackEffectCenterGap = 0.5f;
        constexpr float kPhase2AttackEffectHalfLength =
            kPhase2AttackEffectModelHalfLength * kPhase2AttackEffectScale;
        constexpr float kPhase2AttackEffectSpawnDistance =
            kPhase2AttackEffectHalfLength + kPhase2AttackEffectCenterGap;
        constexpr float kPhase2FastSpinLengthScale = 0.10f;
        constexpr float kPhase2FastSpinHalfLength =
            kPhase2AttackEffectModelHalfLength * kPhase2FastSpinLengthScale;
        constexpr float kPhase2FastSpinSpawnDistance =
            kPhase2FastSpinHalfLength + kPhase2AttackEffectCenterGap;
        constexpr float kPhase2FastSpinAttackRadius =
            kPhase2FastSpinSpawnDistance + kPhase2FastSpinHalfLength;
        constexpr float kPhase2EightWayTelegraphDuration = 2.00f;
        constexpr float kPhase2RainTelegraphDuration = 0.75f;
        constexpr float kPhase2SideSweepTelegraphDuration = 1.25f;
        constexpr float kPhase2FastSpinTelegraphDuration = 1.25f;
        constexpr float kPhase2TelegraphBlinkLeadTime = 0.6f;
        constexpr float kPhase2TelegraphBlinkInterval = 0.08f;
        constexpr float kPhase2WeaponHideBeforeTelegraph = 0.75f;
        constexpr float kPhase2AttackRotationDelay = 2.50f;
        constexpr float kPhase2AttackTotalDuration = 6.75f;
        constexpr float kPhase2FastSpinTotalDuration = 2.75f;
        constexpr float kPhase2AttackRotationEaseInDuration = 1.5f;
        constexpr float kPhase2FastSpinRotationEaseInDuration = 0.20f;
        constexpr float kPhase2RainAttackDuration = 1.0f;
        constexpr float kPhase2RainStartY = 14.0f;
        constexpr float kPhase2RainEndY = -14.0f;
        constexpr float kPhase2SideSweepStartX = 28.0f;
        constexpr float kPhase2SideSweepEndX = -28.0f;
        constexpr float kPhase2SideSweepMinimumY = -11.0f;
        constexpr float kPhase2SideSweepMaximumY = 11.0f;
        constexpr float kPhase2RainLengthScale = 0.10f;
        constexpr float kPhase2TelegraphWidthScale = 0.40f;
        constexpr float kPhase2EightWayTelegraphLengthScale = 0.30f;
        constexpr float kPhase2EightWayTelegraphCenterGap = 3.0f;
        constexpr float kPhase2EffectRenderZ = -1.0f;
        constexpr float kPhase2NextAttackDelay = 2.5f;
        constexpr float kPhase2TeleportMinimumInterval = 8.0f;
        constexpr float kPhase2TeleportMaximumInterval = 10.0f;
        constexpr float kPhase2TeleportEffectDuration = 0.2f;
        constexpr float kPhase2CloneHealthThreshold = 100.0f;
        constexpr float kPhase2CloneOpacity = 0.35f;
        constexpr float kPhase2CloneTelegraphExtension = 0.4f;
        constexpr float kPhase2CloneEightWayTotalDuration = 4.5f;
        constexpr Vector3 kPhase2ClonePosition = { 0.0f, 0.0f, 0.0f };
        constexpr std::array<Vector3, 5> kPhase2TeleportPositions = {
            Vector3{ 0.0f, 0.0f, 0.0f },
            Vector3{ -24.0f, 6.0f, 0.0f },
            Vector3{ 24.0f, 6.0f, 0.0f },
            Vector3{ -24.0f, -6.0f, 0.0f },
            Vector3{ 24.0f, -6.0f, 0.0f },
        };
        constexpr bool kDrawSkyDome = true;
        constexpr char kUiTexturePath[] = "MTEngine/Assets/Resources/Textures/UI/HUD/UI.png";
        constexpr char kEnemyHpTexturePath[] = "MTEngine/Assets/Resources/Textures/UI/HUD/enemyHP_UI.png";
        constexpr char kPlayerHpTexturePath[] = "MTEngine/Assets/Resources/Textures/UI/HUD/playerHP_UI.png";
        constexpr char kBulletCountTexturePath[] = "MTEngine/Assets/Resources/Textures/UI/HUD/BulletCountUI.png";
        constexpr char kGameClearTexturePath[] =
            "MTEngine/Assets/Resources/Textures/Screens/GameClear.png";
        constexpr char kGameOverTexturePath[] =
            "MTEngine/Assets/Resources/Textures/Screens/GameOver.png";
        constexpr char kGoNightmareTexturePath[] =
            "MTEngine/Assets/Resources/Textures/Screens/goNightMare.png";
        constexpr char kTitleControllerTexturePath[] =
            "MTEngine/Assets/Resources/Textures/Screens/controller.png";
        constexpr char kTitleLogoTexturePath[] =
            "MTEngine/Assets/Resources/Textures/Screens/Title.png";
        constexpr char kTitleNightmareTexturePath[] =
            "MTEngine/Assets/Resources/Textures/Screens/nightmare.png";
        constexpr char kTitleGameStartTexturePath[] =
            "MTEngine/Assets/Resources/Textures/Screens/GameStart.png";
        constexpr char kTitleToRankingTexturePath[] =
            "MTEngine/Assets/Resources/Textures/Screens/toRanking.png";
        constexpr char kRankingTexturePath[] =
            "MTEngine/Assets/Resources/Textures/Screens/Ranking/ranking.png";
        constexpr char kNightmareRankingTexturePath[] =
            "MTEngine/Assets/Resources/Textures/Screens/Ranking/Nranking.png";
        constexpr std::array<const char*, 5> kTutorialButtonTexturePaths = {
            "MTEngine/Assets/Resources/Textures/UI/Buttons/Lstick.png",
            "MTEngine/Assets/Resources/Textures/UI/Buttons/Abutton.png",
            "MTEngine/Assets/Resources/Textures/UI/Buttons/RTbutton.png",
            "MTEngine/Assets/Resources/Textures/UI/Buttons/InputGauge.png",
            "MTEngine/Assets/Resources/Textures/UI/Buttons/RTbutton.png",
        };
        constexpr char kTutorialStartButtonTexturePath[] =
            "MTEngine/Assets/Resources/Textures/UI/Buttons/startButton.png";
        constexpr char kTutorialStartArrowTexturePath[] =
            "MTEngine/Assets/Resources/Textures/UI/Buttons/Arrow.png";
        constexpr uint32_t kUiTextureWidth = 1280;
        constexpr uint32_t kUiTextureHeight = 720;
        constexpr uint32_t kHpBarWidth = 346;
        constexpr uint32_t kHpBarHeight = 45;
        constexpr Vector3 kEnemyHpBarPosition = { 923.0f, 14.0f, 0.0f };
        constexpr Vector3 kPlayerHpBarPosition = { 13.0f, 661.0f, 0.0f };
        constexpr float kBulletCountSpacing = 35.0f;
        constexpr uint32_t kTimerCharacterSize = 64;
        constexpr float kTimerLeft = 16.0f;
        constexpr float kTimerTop = 16.0f;
        constexpr uint32_t kResultLargeDigitSize = 80;
        constexpr uint32_t kResultLargeColonSize = 64;
        constexpr uint32_t kResultSmallDigitSize = 52;
        constexpr uint32_t kResultSmallColonSize = 42;
        constexpr float kResultTimeTop = 392.0f;
        constexpr float kResultTimeLeft = 375.0f;
        constexpr uint32_t kResultRankingCharacterSize = 76;
        constexpr float kResultRankingTimeLeft = 440.0f;
        constexpr float kResultRankingTop = 292.0f;
        constexpr float kResultRankingRowSpacing = 154.0f;
        constexpr uint32_t kTutorialButtonSize = 80;
        constexpr float kTutorialButtonSpacing = 24.0f;
        constexpr float kTutorialButtonTop = 72.0f;
        constexpr float kTutorialButtonRowWidth =
            static_cast<float>(kTutorialButtonSize) * 4.0f +
            kTutorialButtonSpacing * 3.0f;
        constexpr float kTutorialButtonLeft =
            (static_cast<float>(kUiTextureWidth) - kTutorialButtonRowWidth) * 0.5f;
        constexpr float kTutorialStartDisplaySpacing = 8.0f;
        constexpr float kTutorialStartDisplayMargin = 32.0f;
        constexpr float kTutorialStartDisplayWidth =
            static_cast<float>(kTutorialButtonSize) * 2.0f +
            kTutorialStartDisplaySpacing;
        constexpr float kTutorialMoveDistance = 4.0f;
        constexpr float kTutorialChargeSecondsPerAmmo = 0.25f;
        constexpr int32_t kTutorialFullChargeAmmo = 10;
        constexpr float kTutorialFullChargeDuration =
            kTutorialChargeSecondsPerAmmo *
            static_cast<float>(kTutorialFullChargeAmmo - 1);
        constexpr float kSkyDomeRotationSpeed = 0.025f;
        constexpr float kPlayerInvincibilityDuration = 0.75f;
        constexpr float kHitFlashDuration = 0.15f;
        constexpr float kPlayerBlinkInterval = 0.08f;
        constexpr float kPlayerDeathAnimationDuration = 1.6f;
        constexpr float kPlayerDeathInitialUpwardSpeed = 11.0f;
        constexpr float kPlayerDeathGravity = -20.0f;
        constexpr float kPlayerDeathRotationSpeed =
            std::numbers::pi_v<float> * 2.5f;
        constexpr Vector3 kPlayerEncounterStartPosition{
            -24.69f, -7.20f, 0.0f,
        };
        constexpr float kPlayerEncounterStartRotationY =
            -std::numbers::pi_v<float> / 2.0f;
        constexpr float kPhase1IntroSpawnY = 20.0f;
        constexpr float kPhase1IntroLandingY = 0.0f;
        constexpr float kPhase1IntroDescentDuration = 3.0f;
        constexpr float kPhase1IntroHealthFillStartTime = 1.5f;
        constexpr float kPhase1IntroDuration = 4.0f;
        constexpr float kPhase1DefeatCameraZoomDuration = 0.45f;
        constexpr float kPhase1TransformEnergyBuildStartTime = 0.30f;
        constexpr float kPhase1TransformReturnStartTime = 0.30f;
        constexpr float kPhase1TransformReturnEndTime = 1.20f;
        constexpr float kPhase1TransformCollapseStartTime = 1.20f;
        constexpr float kPhase1TransformModelSwapTime = 1.75f;
        constexpr float kPhase1TransformExpansionEndTime = 2.55f;
        constexpr float kPhase1DefeatAnimationDuration = 2.85f;
        constexpr float kPhaseTransitionCoreScale = 0.14f;
        constexpr float kPhaseTransitionExpandedScale = 1.22f;
        constexpr float kPhaseTransitionRotation =
            std::numbers::pi_v<float>;
        constexpr float kPhase2IntroSettleEndTime = 0.70f;
        constexpr float kPhase2IntroHealthFillStartTime = 0.40f;
        constexpr float kPhase2IntroCameraReturnStartTime = 1.50f;
        constexpr float kPhase2IntroDuration = 2.50f;
        constexpr float kPhase2DefeatShrinkStartTime = 1.8f;
        constexpr float kPhase2DefeatDisappearTime = 2.4f;
        constexpr float kPhase2DefeatCameraReturnStartTime = 2.3f;
        constexpr float kPhase2DefeatAnimationDuration = 3.0f;

        void RemoveOpenBorderCells(
            std::vector<TileMapCell>& cells,
            const TileMap& tileMap) {
            const int32_t bottomRow = tileMap.GetHeight() - 1;
            const int32_t rightColumn = tileMap.GetWidth() - 1;
            std::erase_if(cells, [&](const TileMapCell& cell) {
                if (cell.row == 0) {
                    return true;
                }
                if (cell.row == bottomRow) {
                    return false;
                }
                return cell.column == 0 || cell.column == rightColumn;
            });
        }

        Matrix4x4 MakeScreenSpaceProjectionMatrix() {
            Matrix4x4 projection{};
            projection.m[0][0] = 2.0f / static_cast<float>(kClientWidth);
            projection.m[1][1] = -2.0f / static_cast<float>(kClientHeight);
            projection.m[2][2] = 1.0f;
            projection.m[3][0] = -1.0f;
            projection.m[3][1] = 1.0f;
            projection.m[3][3] = 1.0f;
            return projection;
        }

        Matrix4x4 MakeOrientationMarkerProjectionMatrix() {
            Matrix4x4 projection{};
            projection.m[0][0] = 0.12f;
            projection.m[1][1] = 0.12f;
            projection.m[2][2] = 0.01f;
            projection.m[3][2] = 0.10f;
            projection.m[3][3] = 1.0f;
            return projection;
        }

        bool TryParseTileId(const std::string& text, int32_t& tileId) {
            const size_t first = text.find_first_not_of(" \t");
            if (first == std::string::npos) {
                return false;
            }

            const size_t last = text.find_last_not_of(" \t");
            const char* begin = text.data() + first;
            const char* end = text.data() + last + 1;
            const auto [parsedEnd, error] = std::from_chars(begin, end, tileId);
            return error == std::errc{} && parsedEnd == end;
        }

    }

    bool TileMap::LoadFromCsv(const std::string& filePath) {
        Clear();

        std::ifstream file(filePath);
        if (!file.is_open()) {
            lastError_ = "CSV file could not be opened: " + filePath;
            return false;
        }

        std::string line;
        int32_t csvLineNumber = 0;
        while (std::getline(file, line)) {
            ++csvLineNumber;

            // UTF-8 BOM が先頭に存在しても、最初の数値を正しく読み込めるようにします。
            if (csvLineNumber == 1 && line.starts_with("\xEF\xBB\xBF")) {
                line.erase(0, 3);
            }

            const size_t first = line.find_first_not_of(" \t\r");
            if (first == std::string::npos || line[first] == '#') {
                continue;
            }

            std::vector<int32_t> row;
            std::stringstream stream(line);
            std::string field;
            while (std::getline(stream, field, ',')) {
                int32_t tileId = kEmptyTileId;
                if (!TryParseTileId(field, tileId)) {
                    Clear();
                    lastError_ = "Invalid tile ID at CSV line " + std::to_string(csvLineNumber) + ".";
                    return false;
                }
                row.push_back(tileId);
            }

            if (row.empty()) {
                Clear();
                lastError_ = "Empty row at CSV line " + std::to_string(csvLineNumber) + ".";
                return false;
            }

            if (width_ == 0) {
                width_ = static_cast<int32_t>(row.size());
            }
            else if (static_cast<int32_t>(row.size()) != width_) {
                Clear();
                lastError_ = "Column count differs at CSV line " + std::to_string(csvLineNumber) + ".";
                return false;
            }

            tiles_.insert(tiles_.end(), row.begin(), row.end());
            ++height_;
        }

        if (tiles_.empty()) {
            lastError_ = "CSV contains no tile rows: " + filePath;
            return false;
        }

        return true;
    }

    void TileMap::Clear() {
        width_ = 0;
        height_ = 0;
        tiles_.clear();
        lastError_.clear();
    }

    int32_t TileMap::GetTileId(int32_t column, int32_t row) const {
        if (column < 0 || row < 0 || column >= width_ || row >= height_) {
            return kEmptyTileId;
        }
        return tiles_[static_cast<size_t>(row) * width_ + column];
    }

    std::vector<TileMapCell> TileMap::CreateCells(float tileSize, const Vector3& origin) const {
        std::vector<TileMapCell> cells;
        if (tileSize <= 0.0f) {
            return cells;
        }

        cells.reserve(tiles_.size());
        for (int32_t row = 0; row < height_; ++row) {
            for (int32_t column = 0; column < width_; ++column) {
                const int32_t tileId = GetTileId(column, row);
                if (tileId == kEmptyTileId) {
                    continue;
                }

                cells.push_back({
                    tileId,
                    column,
                    row,
                    {
                        origin.x + static_cast<float>(column) * tileSize,
                        origin.y + static_cast<float>(height_ - 1 - row) * tileSize,
                        origin.z,
                    },
                });
            }
        }
        return cells;
    }

    bool GameScene::Initialize(
        Microsoft::WRL::ComPtr<ID3D12Device> device,
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
        DescriptorHeapManager* srvHeapManager,
        const std::string& csvFilePath)
    {
        device_ = device;
        if (!tileMap_.LoadFromCsv(csvFilePath)) {
            return false;
        }

        constexpr float kTileSize = 2.0f;
        const Vector3 mapOrigin = {
            -static_cast<float>(tileMap_.GetWidth() - 1) * kTileSize * 0.5f,
            -static_cast<float>(tileMap_.GetHeight() - 1) * kTileSize * 0.5f,
            0.0f,
        };
        cells_ = tileMap_.CreateCells(kTileSize, mapOrigin);
        RemoveOpenBorderCells(cells_, tileMap_);

        const ModelData skyDomeModelData = Model::LoadObjFile(
            "MTEngine/Assets/Resources/Models/SkyDome", "SkyDome.obj");
        if (!skyDomeModel_.Initialize(device, skyDomeModelData)) {
            return false;
        }

        const ModelData cubeModelData = Model::LoadObjFile("MTEngine/Assets/Resources/Models/Cube", "cube.obj");
        if (!cubeModel_.Initialize(device, cubeModelData)) {
            return false;
        }

        const ModelData bulletModelData = Model::LoadObjFile(
            "MTEngine/Assets/Resources/Models/Bullet", "bullet.obj");
        if (!bulletModel_.Initialize(device, bulletModelData)) {
            return false;
        }

        const ModelData playerModelData = Model::LoadObjFile("MTEngine/Assets/Resources/Models/Player", "player.obj");
        if (!playerModel_.Initialize(device, playerModelData)) {
            return false;
        }

        const ModelData enemyModelData = Model::LoadObjFile("MTEngine/Assets/Resources/Models/EnemyPhase1", "enemy.obj");
        if (!enemyModel_.Initialize(device, enemyModelData)) {
            return false;
        }

        const ModelData enemyPhase2BodyModelData = Model::LoadObjFile(
            "MTEngine/Assets/Resources/Models/EnemyPhase2", "enemy2_body.obj");
        const ModelData enemyPhase2Body2ModelData = Model::LoadObjFile(
            "MTEngine/Assets/Resources/Models/EnemyPhase2", "enemy2_body2.obj");
        const ModelData enemyPhase2WeaponModelData = Model::LoadObjFile(
            "MTEngine/Assets/Resources/Models/EnemyPhase2", "enemy2_weapon.obj");
        if (!enemyPhase2BodyModel_.Initialize(device, enemyPhase2BodyModelData) ||
            !enemyPhase2Body2Model_.Initialize(device, enemyPhase2Body2ModelData) ||
            !enemyPhase2WeaponModel_.Initialize(device, enemyPhase2WeaponModelData)) {
            return false;
        }

        const ModelData enemyAttackEffectModelData = Model::LoadObjFile(
            "MTEngine/Assets/Resources/Models/EnemyAttackEffect", "enemyAttackEffect.obj");
        if (!enemyAttackEffectModel_.Initialize(device, enemyAttackEffectModelData)) {
            return false;
        }

        textureManager_.Initialize(device, srvHeapManager);
        for (const MeshData& mesh : skyDomeModelData.meshes) {
            if (!mesh.material.textureFilePath.empty()) {
                textureManager_.Load(mesh.material.textureFilePath, commandList);
            }
        }
        for (const MeshData& mesh : cubeModelData.meshes) {
            if (!mesh.material.textureFilePath.empty()) {
                textureManager_.Load(mesh.material.textureFilePath, commandList);
            }
        }
        for (const MeshData& mesh : bulletModelData.meshes) {
            if (!mesh.material.textureFilePath.empty()) {
                textureManager_.Load(mesh.material.textureFilePath, commandList);
            }
        }
        for (const MeshData& mesh : playerModelData.meshes) {
            if (!mesh.material.textureFilePath.empty()) {
                textureManager_.Load(mesh.material.textureFilePath, commandList);
            }
        }
        for (const MeshData& mesh : enemyModelData.meshes) {
            if (!mesh.material.textureFilePath.empty()) {
                textureManager_.Load(mesh.material.textureFilePath, commandList);
            }
        }
        const ModelData* enemyPhase2ModelParts[] = {
            &enemyPhase2BodyModelData,
            &enemyPhase2Body2ModelData,
            &enemyPhase2WeaponModelData,
        };
        for (const ModelData* modelPart : enemyPhase2ModelParts) {
            for (const MeshData& mesh : modelPart->meshes) {
                if (!mesh.material.textureFilePath.empty()) {
                    textureManager_.Load(mesh.material.textureFilePath, commandList);
                }
            }
        }
        for (const MeshData& mesh : enemyAttackEffectModelData.meshes) {
            if (!mesh.material.textureFilePath.empty()) {
                textureManager_.Load(mesh.material.textureFilePath, commandList);
            }
        }
        textureManager_.Load(kUiTexturePath, commandList);
        textureManager_.Load(kEnemyHpTexturePath, commandList);
        textureManager_.Load(kPlayerHpTexturePath, commandList);
        textureManager_.Load(kBulletCountTexturePath, commandList);
        textureManager_.Load(kGameClearTexturePath, commandList);
        textureManager_.Load(kGameOverTexturePath, commandList);
        textureManager_.Load(kGoNightmareTexturePath, commandList);
        textureManager_.Load(kTitleControllerTexturePath, commandList);
        textureManager_.Load(kTitleLogoTexturePath, commandList);
        textureManager_.Load(kTitleNightmareTexturePath, commandList);
        textureManager_.Load(kTitleGameStartTexturePath, commandList);
        textureManager_.Load(kTitleToRankingTexturePath, commandList);
        textureManager_.Load(kRankingTexturePath, commandList);
        textureManager_.Load(kNightmareRankingTexturePath, commandList);
        for (const char* buttonTexturePath : kTutorialButtonTexturePaths) {
            textureManager_.Load(buttonTexturePath, commandList);
        }
        textureManager_.Load(kTutorialStartButtonTexturePath, commandList);
        textureManager_.Load(kTutorialStartArrowTexturePath, commandList);
        uiSprite_.Initialize(
            device,
            kUiTextureWidth,
            kUiTextureHeight,
            textureManager_.GetGPUDescriptorHandle(kUiTexturePath));
        enemyHpSprite_.Initialize(
            device,
            kUiTextureWidth,
            kUiTextureHeight,
            textureManager_.GetGPUDescriptorHandle(kEnemyHpTexturePath));

        playerHpSprite_.Initialize(
            device,
            kUiTextureWidth,
            kUiTextureHeight,
            textureManager_.GetGPUDescriptorHandle(kPlayerHpTexturePath));
        gameClearSprite_.Initialize(
            device,
            kUiTextureWidth,
            kUiTextureHeight,
            textureManager_.GetGPUDescriptorHandle(kGameClearTexturePath));
        gameOverSprite_.Initialize(
            device,
            kUiTextureWidth,
            kUiTextureHeight,
            textureManager_.GetGPUDescriptorHandle(kGameOverTexturePath));
        goNightmareSprite_.Initialize(
            device,
            kUiTextureWidth,
            kUiTextureHeight,
            textureManager_.GetGPUDescriptorHandle(kGoNightmareTexturePath));
        titleControllerSprite_.Initialize(
            device,
            kUiTextureWidth,
            kUiTextureHeight,
            textureManager_.GetGPUDescriptorHandle(
                kTitleControllerTexturePath));
        titleLogoSprite_.Initialize(
            device,
            kUiTextureWidth,
            kUiTextureHeight,
            textureManager_.GetGPUDescriptorHandle(
                kTitleLogoTexturePath));
        titleNightmareSprite_.Initialize(
            device,
            kUiTextureWidth,
            kUiTextureHeight,
            textureManager_.GetGPUDescriptorHandle(
                kTitleNightmareTexturePath));
        titleGameStartSprite_.Initialize(
            device,
            kUiTextureWidth,
            kUiTextureHeight,
            textureManager_.GetGPUDescriptorHandle(
                kTitleGameStartTexturePath));
        titleToRankingSprite_.Initialize(
            device,
            kUiTextureWidth,
            kUiTextureHeight,
            textureManager_.GetGPUDescriptorHandle(
                kTitleToRankingTexturePath));
        rankingBackgroundSprite_.Initialize(
            device,
            kUiTextureWidth,
            kUiTextureHeight,
            textureManager_.GetGPUDescriptorHandle(kRankingTexturePath));
        nightmareRankingBackgroundSprite_.Initialize(
            device,
            kUiTextureWidth,
            kUiTextureHeight,
            textureManager_.GetGPUDescriptorHandle(
                kNightmareRankingTexturePath));
        for (size_t buttonIndex = 0;
            buttonIndex < tutorialButtonSprites_.size();
            ++buttonIndex) {
            tutorialButtonSprites_[buttonIndex].Initialize(
                device,
                kTutorialButtonSize,
                kTutorialButtonSize,
                textureManager_.GetGPUDescriptorHandle(
                    kTutorialButtonTexturePaths[buttonIndex]));
            tutorialButtonSprites_[buttonIndex].transform.translate = {
                kTutorialButtonLeft + static_cast<float>(
                    buttonIndex < 4 ? buttonIndex : 3) *
                    (static_cast<float>(kTutorialButtonSize) +
                        kTutorialButtonSpacing),
                kTutorialButtonTop,
                0.0f,
            };
        }
        tutorialStartButtonSprite_.Initialize(
            device,
            kTutorialButtonSize,
            kTutorialButtonSize,
            textureManager_.GetGPUDescriptorHandle(
                kTutorialStartButtonTexturePath));
        goNightmareStartButtonSprite_.Initialize(
            device,
            kTutorialButtonSize,
            kTutorialButtonSize,
            textureManager_.GetGPUDescriptorHandle(
                kTutorialStartButtonTexturePath));
        tutorialStartArrowSprite_.Initialize(
            device,
            kTutorialButtonSize,
            kTutorialButtonSize,
            textureManager_.GetGPUDescriptorHandle(
                kTutorialStartArrowTexturePath));
        tutorialSkipGaugeSprite_.Initialize(
            device,
            kTutorialButtonSize,
            kTutorialButtonSize,
            textureManager_.GetGPUDescriptorHandle(
                kTutorialButtonTexturePaths[3]));
        const D3D12_GPU_DESCRIPTOR_HANDLE bulletCountTextureHandle =
            textureManager_.GetGPUDescriptorHandle(kBulletCountTexturePath);
        for (size_t bulletIndex = 0; bulletIndex < bulletCountSprites_.size(); ++bulletIndex) {
            bulletCountSprites_[bulletIndex].Initialize(
                device,
                kUiTextureWidth,
                kUiTextureHeight,
                bulletCountTextureHandle);
            bulletCountSprites_[bulletIndex].transform.translate.x =
                static_cast<float>(bulletIndex) * kBulletCountSpacing;
        }

        for (size_t digit = 0; digit < timerDigitTextureHandles_.size(); ++digit) {
            const std::string digitTexturePath =
                "MTEngine/Assets/Resources/Textures/UI/Digits/" + std::to_string(digit) + ".png";
            textureManager_.Load(digitTexturePath, commandList);
            timerDigitTextureHandles_[digit] =
                textureManager_.GetGPUDescriptorHandle(digitTexturePath);
        }
        constexpr size_t kTimerDigitCharacterPositions[] = { 0, 1, 3, 4 };
        for (size_t digitIndex = 0; digitIndex < timerDigitSprites_.size(); ++digitIndex) {
            timerDigitSprites_[digitIndex].Initialize(
                device,
                kTimerCharacterSize,
                kTimerCharacterSize,
                timerDigitTextureHandles_[0]);
            timerDigitSprites_[digitIndex].transform.translate = {
                kTimerLeft + static_cast<float>(kTimerDigitCharacterPositions[digitIndex]) *
                    static_cast<float>(kTimerCharacterSize),
                kTimerTop,
                0.0f,
            };
        }
        constexpr char kTimerColonTexturePath[] = "MTEngine/Assets/Resources/Textures/UI/Digits/colon.png";
        textureManager_.Load(kTimerColonTexturePath, commandList);
        timerColonSprite_.Initialize(
            device,
            kTimerCharacterSize,
            kTimerCharacterSize,
            textureManager_.GetGPUDescriptorHandle(kTimerColonTexturePath));
        timerColonSprite_.transform.translate = {
            kTimerLeft + 2.0f * static_cast<float>(kTimerCharacterSize),
            kTimerTop,
            0.0f,
        };

        constexpr float kResultDigitLeftPositions[] = {
            kResultTimeLeft,
            kResultTimeLeft + static_cast<float>(kResultLargeDigitSize),
            kResultTimeLeft + 2.0f * static_cast<float>(kResultLargeDigitSize) +
                static_cast<float>(kResultLargeColonSize),
            kResultTimeLeft + 3.0f * static_cast<float>(kResultLargeDigitSize) +
                static_cast<float>(kResultLargeColonSize),
            kResultTimeLeft + 4.0f * static_cast<float>(kResultLargeDigitSize) +
                static_cast<float>(kResultLargeColonSize) +
                static_cast<float>(kResultSmallColonSize),
            kResultTimeLeft + 4.0f * static_cast<float>(kResultLargeDigitSize) +
                static_cast<float>(kResultLargeColonSize) +
                static_cast<float>(kResultSmallColonSize) +
                static_cast<float>(kResultSmallDigitSize),
        };
        for (size_t digitIndex = 0;
            digitIndex < resultTimeDigitSprites_.size();
            ++digitIndex) {
            const bool isSmallDigit = digitIndex >= 4;
            const uint32_t digitSize = isSmallDigit
                ? kResultSmallDigitSize
                : kResultLargeDigitSize;
            resultTimeDigitSprites_[digitIndex].Initialize(
                device,
                digitSize,
                digitSize,
                timerDigitTextureHandles_[0]);
            resultTimeDigitSprites_[digitIndex].transform.translate = {
                kResultDigitLeftPositions[digitIndex],
                kResultTimeTop + (isSmallDigit
                    ? static_cast<float>(
                        kResultLargeDigitSize - kResultSmallDigitSize)
                    : 0.0f),
                0.0f,
            };
        }

        const D3D12_GPU_DESCRIPTOR_HANDLE colonTextureHandle =
            textureManager_.GetGPUDescriptorHandle(kTimerColonTexturePath);
        resultTimeColonSprites_[0].Initialize(
            device,
            kResultLargeColonSize,
            kResultLargeDigitSize,
            colonTextureHandle);
        resultTimeColonSprites_[0].transform.translate = {
            kResultTimeLeft + 2.0f * static_cast<float>(kResultLargeDigitSize),
            kResultTimeTop,
            0.0f,
        };
        resultTimeColonSprites_[1].Initialize(
            device,
            kResultSmallColonSize,
            kResultSmallDigitSize,
            colonTextureHandle);
        resultTimeColonSprites_[1].transform.translate = {
            kResultTimeLeft + 4.0f * static_cast<float>(kResultLargeDigitSize) +
                static_cast<float>(kResultLargeColonSize),
            kResultTimeTop + static_cast<float>(
                kResultLargeDigitSize - kResultSmallDigitSize),
            0.0f,
        };

        constexpr size_t kResultRankingCount = 3;
        for (size_t rankIndex = 0;
            rankIndex < kResultRankingCount;
            ++rankIndex) {
            const float rowTop = kResultRankingTop +
                static_cast<float>(rankIndex) * kResultRankingRowSpacing;
            for (size_t digitIndex = 0;
                digitIndex < resultRankingDigitSprites_[rankIndex].size();
                ++digitIndex) {
                const size_t precedingColonCount = digitIndex / 2;
                const float digitLeft = kResultRankingTimeLeft +
                    static_cast<float>(
                        digitIndex + precedingColonCount) *
                        static_cast<float>(kResultRankingCharacterSize);
                resultRankingDigitSprites_[rankIndex][digitIndex].Initialize(
                    device,
                    kResultRankingCharacterSize,
                    kResultRankingCharacterSize,
                    timerDigitTextureHandles_[0]);
                resultRankingDigitSprites_[rankIndex][digitIndex]
                    .transform.translate = {
                        digitLeft,
                        rowTop,
                        0.0f,
                    };
            }

            for (size_t colonIndex = 0;
                colonIndex < resultRankingColonSprites_[rankIndex].size();
                ++colonIndex) {
                const float colonLeft = kResultRankingTimeLeft +
                    static_cast<float>(2 + colonIndex * 3) *
                        static_cast<float>(kResultRankingCharacterSize);
                resultRankingColonSprites_[rankIndex][colonIndex].Initialize(
                    device,
                    kResultRankingCharacterSize,
                    kResultRankingCharacterSize,
                    colonTextureHandle);
                resultRankingColonSprites_[rankIndex][colonIndex]
                    .transform.translate = {
                        colonLeft,
                        rowTop,
                        0.0f,
                    };
            }
        }

        if (!skyDomeMaterialConstantBuffer_.Initialize(device) ||
            !skyDomeTransformConstantBuffer_.Initialize(device) ||
            !materialConstantBuffer_.Initialize(device) ||
            !playerMaterialConstantBuffer_.Initialize(device) ||
            !playerTransformConstantBuffer_.Initialize(device) ||
            !enemyMaterialConstantBuffer_.Initialize(device) ||
            !phase2CloneMaterialConstantBuffer_.Initialize(device) ||
            !enemyAttackEffectMaterialConstantBuffer_.Initialize(device) ||
            !phase2CloneAttackEffectMaterialConstantBuffer_.Initialize(device) ||
            !projectileMaterialConstantBuffer_.Initialize(device) ||
            !chargingProjectileTransformConstantBuffer_.Initialize(device) ||
            !selectionMaterialConstantBuffer_.Initialize(device) ||
            !phase2CloneTransformConstantBuffer_.Initialize(device) ||
            !lightConstantBuffer_.Initialize(device)) {
            return false;
        }

        transformConstantBuffers_.clear();
        transformConstantBuffers_.reserve(cells_.size());
        for (size_t cellIndex = 0; cellIndex < cells_.size(); ++cellIndex) {
            auto transformConstantBuffer = std::make_unique<TransformationMatrixConstantBuffer>();
            if (!transformConstantBuffer->Initialize(device)) {
                return false;
            }
            transformConstantBuffers_.push_back(std::move(transformConstantBuffer));
        }

        // The enemy model's bottom is about 0.8 units below its origin at this scale.
        // Place one enemy on the floor.
        enemyPositions_ = {
            { 8.0f, -7.188f, 0.0f },
        };
        enemyVerticalVelocities_.assign(enemyPositions_.size(), 0.0f);
        enemyHealths_.assign(enemyPositions_.size(), 100.0f);
        enemyBehaviors_.assign(enemyPositions_.size(), EnemyBehavior::Move);
        enemyBehaviorTimers_.clear();
        enemyBehaviorTimers_.reserve(enemyPositions_.size());
        for (size_t enemyIndex = 0; enemyIndex < enemyPositions_.size(); ++enemyIndex) {
            enemyBehaviorTimers_.push_back(GetRandomEnemyBehaviorDuration(EnemyBehavior::Move));
        }
        enemyMoveDirections_.assign(enemyPositions_.size(), -1.0f);
        enemyMoveSpeeds_.assign(enemyPositions_.size(), 0.0f);
        enemyTargetMoveSpeeds_.assign(enemyPositions_.size(), 0.0f);
        enemySpeedVariationTimers_.assign(enemyPositions_.size(), 0.0f);
        enemyFloatDirections_.clear();
        enemyFloatDirections_.reserve(enemyPositions_.size());
        for (size_t enemyIndex = 0; enemyIndex < enemyPositions_.size(); ++enemyIndex) {
            enemyFloatDirections_.push_back(GetRandomEnemyFloatDirection());
        }
        enemyGrounded_.assign(enemyPositions_.size(), false);
        enemyAttackCooldowns_.assign(enemyPositions_.size(), 0.0f);
        enemyAttackHitPlayers_.assign(enemyPositions_.size(), false);
        enemyRepeatDashUsed_.assign(enemyPositions_.size(), false);
        enemyAttackDirections_.assign(enemyPositions_.size(), { 1.0f, 0.0f, 0.0f });
        enemyAttackStartPositions_ = enemyPositions_;
        enemyTransformConstantBuffers_.clear();
        enemyTransformConstantBuffers_.reserve(enemyPositions_.size());
        enemyPhase2WeaponTransformConstantBuffers_.clear();
        enemyPhase2WeaponTransformConstantBuffers_.reserve(enemyPositions_.size());
        for (size_t enemyIndex = 0; enemyIndex < enemyPositions_.size(); ++enemyIndex) {
            auto transformConstantBuffer = std::make_unique<TransformationMatrixConstantBuffer>();
            if (!transformConstantBuffer->Initialize(device)) {
                return false;
            }
            enemyTransformConstantBuffers_.push_back(std::move(transformConstantBuffer));

            auto weaponTransformConstantBuffer =
                std::make_unique<TransformationMatrixConstantBuffer>();
            if (!weaponTransformConstantBuffer->Initialize(device)) {
                return false;
            }
            enemyPhase2WeaponTransformConstantBuffers_.push_back(
                std::move(weaponTransformConstantBuffer));
        }

        projectileTransformConstantBuffers_.clear();
        projectileTransformConstantBuffers_.reserve(kMaxProjectiles);
        for (size_t projectileIndex = 0; projectileIndex < kMaxProjectiles; ++projectileIndex) {
            auto transformConstantBuffer = std::make_unique<TransformationMatrixConstantBuffer>();
            if (!transformConstantBuffer->Initialize(device)) {
                return false;
            }
            projectileTransformConstantBuffers_.push_back(std::move(transformConstantBuffer));
        }

        for (TransformationMatrixConstantBuffer& transformConstantBuffer : enemyAttackEffectTransformConstantBuffers_) {
            if (!transformConstantBuffer.Initialize(device)) {
                return false;
            }
        }
        for (TransformationMatrixConstantBuffer& transformConstantBuffer : phase2CloneAttackEffectTransformConstantBuffers_) {
            if (!transformConstantBuffer.Initialize(device)) {
                return false;
            }
        }

        for (size_t axisIndex = 0; axisIndex < axisMaterialConstantBuffers_.size(); ++axisIndex) {
            if (!axisMaterialConstantBuffers_[axisIndex].Initialize(device) ||
                !axisTransformConstantBuffers_[axisIndex].Initialize(device)) {
                return false;
            }
        }

        skyDomeMaterialConstantBuffer_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        skyDomeMaterialConstantBuffer_->enabledLighting = 0;
        skyDomeMaterialConstantBuffer_->uvTransform = MakeIdentityMatrix();
        skyDomeMaterialConstantBuffer_->lightingMode = 0;
        skyDomeMaterialConstantBuffer_->useTexture = 1;
        skyDomeMaterialConstantBuffer_->isSelected = 0;

        materialConstantBuffer_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        materialConstantBuffer_->enabledLighting = 0;
        materialConstantBuffer_->uvTransform = MakeIdentityMatrix();
        materialConstantBuffer_->lightingMode = 2;
        materialConstantBuffer_->useTexture = 1;
        materialConstantBuffer_->isSelected = 0;

        playerMaterialConstantBuffer_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        playerMaterialConstantBuffer_->enabledLighting = 0;
        playerMaterialConstantBuffer_->uvTransform = MakeIdentityMatrix();
        playerMaterialConstantBuffer_->lightingMode = 2;
        playerMaterialConstantBuffer_->useTexture = 1;
        playerMaterialConstantBuffer_->isSelected = 0;

        enemyMaterialConstantBuffer_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        enemyMaterialConstantBuffer_->enabledLighting = 0;
        enemyMaterialConstantBuffer_->uvTransform = MakeIdentityMatrix();
        enemyMaterialConstantBuffer_->lightingMode = 2;
        enemyMaterialConstantBuffer_->useTexture = 1;
        enemyMaterialConstantBuffer_->isSelected = 0;

        phase2CloneMaterialConstantBuffer_->color = {
            0.65f, 0.90f, 1.0f, kPhase2CloneOpacity,
        };
        phase2CloneMaterialConstantBuffer_->enabledLighting = 0;
        phase2CloneMaterialConstantBuffer_->uvTransform = MakeIdentityMatrix();
        phase2CloneMaterialConstantBuffer_->lightingMode = 2;
        phase2CloneMaterialConstantBuffer_->useTexture = 1;
        phase2CloneMaterialConstantBuffer_->isSelected = 0;

        enemyAttackEffectMaterialConstantBuffer_->color = { 1.0f, 0.15f, 0.05f, 1.0f };
        enemyAttackEffectMaterialConstantBuffer_->enabledLighting = 0;
        enemyAttackEffectMaterialConstantBuffer_->uvTransform = MakeIdentityMatrix();
        enemyAttackEffectMaterialConstantBuffer_->lightingMode = 0;
        enemyAttackEffectMaterialConstantBuffer_->useTexture = 0;
        enemyAttackEffectMaterialConstantBuffer_->isSelected = 0;

        phase2CloneAttackEffectMaterialConstantBuffer_->color = {
            1.0f, 0.15f, 0.05f, 1.0f,
        };
        phase2CloneAttackEffectMaterialConstantBuffer_->enabledLighting = 0;
        phase2CloneAttackEffectMaterialConstantBuffer_->uvTransform =
            MakeIdentityMatrix();
        phase2CloneAttackEffectMaterialConstantBuffer_->lightingMode = 0;
        phase2CloneAttackEffectMaterialConstantBuffer_->useTexture = 0;
        phase2CloneAttackEffectMaterialConstantBuffer_->isSelected = 0;

        projectileMaterialConstantBuffer_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        projectileMaterialConstantBuffer_->enabledLighting = 0;
        projectileMaterialConstantBuffer_->uvTransform = MakeIdentityMatrix();
        projectileMaterialConstantBuffer_->lightingMode = 0;
        projectileMaterialConstantBuffer_->useTexture = 1;
        projectileMaterialConstantBuffer_->isSelected = 0;

        selectionMaterialConstantBuffer_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        selectionMaterialConstantBuffer_->enabledLighting = 0;
        selectionMaterialConstantBuffer_->uvTransform = MakeIdentityMatrix();
        selectionMaterialConstantBuffer_->lightingMode = 0;
        selectionMaterialConstantBuffer_->useTexture = 1;
        selectionMaterialConstantBuffer_->isSelected = 1;

        const Vector4 axisColors[] = {
            { 0.95f, 0.15f, 0.15f, 1.0f }, // +X
            { 0.15f, 0.95f, 0.25f, 1.0f }, // +Y
            { 0.20f, 0.45f, 1.00f, 1.0f }, // +Z
        };
        for (size_t axisIndex = 0; axisIndex < axisMaterialConstantBuffers_.size(); ++axisIndex) {
            MaterialConstantBuffer& axisMaterial = axisMaterialConstantBuffers_[axisIndex];
            axisMaterial->color = axisColors[axisIndex];
            axisMaterial->enabledLighting = 0;
            axisMaterial->uvTransform = MakeIdentityMatrix();
            axisMaterial->lightingMode = 0;
            axisMaterial->useTexture = 0;
            axisMaterial->isSelected = 0;
        }

        lightConstantBuffer_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        lightConstantBuffer_->direction = { 0.3f, -1.0f, 0.2f };
        lightConstantBuffer_->intensity = 1.0f;

        Reset();
        return true;
    }

    void GameScene::Reset() {
        isTutorialMode_ = false;
        tutorialStep_ = TutorialStep::Move;
        tutorialStartPlayerX_ = 0.0f;
        tutorialIconAnimationTime_ = 0.0f;
        tutorialAttackHintTimer_ = 0.0f;
        hasTutorialJumpStarted_ = false;
        suppressPlayerInputForOneFrame_ = false;
        tutorialSkipHoldProgress_ = 0.0f;
        // The player model's bottom is 0.8 units below its origin at this scale.
        // Start on the floor inside the map.
        playerPosition_ = kPlayerEncounterStartPosition;
        playerRotationY_ = kPlayerEncounterStartRotationY;
        playerTargetRotationY_ = playerRotationY_;
        playerDeathRotationX_ = 0.0f;
        playerDeathAnimationTimer_ = 0.0f;
        isPlayerDeathAnimationActive_ = false;
        isPlayerDeathAnimationFinished_ = false;
        playerHorizontalVelocity_ = 0.0f;
        playerVerticalVelocity_ = 0.0f;
        playerHealth_ = GetPlayerMaxHealth();
        bossPhase_ = 1;
        LoadStageForPhase(bossPhase_);
        enemyContactDamageCooldown_ = 0.0f;
        playerInvincibilityTimer_ = 0.0f;
        playerHitFlashTimer_ = 0.0f;
        enemyHitFlashTimer_ = 0.0f;
        playerChargeTime_ = 0.0f;
        isPlayerCharging_ = false;
        firedProjectileAmmo_ = 0;
        teleportSoundRequested_ = false;
        dashSoundRequested_ = false;
        alertSoundRequested_ = false;
        fallSoundRequested_ = false;
        damageSoundRequested_ = false;
        enemyDamageSoundRequested_ = false;
        isEnemyDefeatAnimationActive_ = false;
        isEnemyDefeatAnimationFinished_ = false;
        enemyDefeatAnimationTimer_ = 0.0f;
        isPhase2IntroActive_ = false;
        phase2IntroElapsedTime_ = 0.0f;
        phase2IntroStartPosition_ = {};
        isPlayerGrounded_ = true;
        enemyPositions_ = { { 0.0f, kPhase1IntroSpawnY, 0.0f } };
        enemyVerticalVelocities_.assign(enemyPositions_.size(), 0.0f);
        enemyHealths_.assign(enemyPositions_.size(), 0.0f);
        enemyBehaviors_.assign(enemyPositions_.size(), EnemyBehavior::Stop);
        enemyBehaviorTimers_.clear();
        enemyBehaviorTimers_.reserve(enemyPositions_.size());
        for (size_t enemyIndex = 0; enemyIndex < enemyPositions_.size(); ++enemyIndex) {
            enemyBehaviorTimers_.push_back(GetRandomEnemyBehaviorDuration(EnemyBehavior::Move));
        }
        enemyMoveDirections_.assign(enemyPositions_.size(), -1.0f);
        enemyMoveSpeeds_.assign(enemyPositions_.size(), 0.0f);
        enemyTargetMoveSpeeds_.assign(enemyPositions_.size(), 0.0f);
        enemySpeedVariationTimers_.assign(enemyPositions_.size(), 0.0f);
        enemyFloatDirections_.clear();
        enemyFloatDirections_.reserve(enemyPositions_.size());
        for (size_t enemyIndex = 0; enemyIndex < enemyPositions_.size(); ++enemyIndex) {
            enemyFloatDirections_.push_back(GetRandomEnemyFloatDirection());
        }
        enemyGrounded_.assign(enemyPositions_.size(), false);
        enemyAttackCooldowns_.assign(enemyPositions_.size(), 0.0f);
        enemyAttackHitPlayers_.assign(enemyPositions_.size(), false);
        enemyRepeatDashUsed_.assign(enemyPositions_.size(), false);
        enemyAttackDirections_.assign(enemyPositions_.size(), { 1.0f, 0.0f, 0.0f });
        enemyAttackStartPositions_ = enemyPositions_;
        projectiles_.assign(kMaxProjectiles, {});
        arePhase2AttackEffectsActive_ = false;
        phase2AttackEffectRotation_ = 0.0f;
        phase2AttackEffectRotationDirection_ = 1.0f;
        phase2AttackEffectElapsedTime_ = 0.0f;
        phase2AttackDamageCooldown_ = 0.0f;
        phase2NextAttackTimer_ =
            kPhase2NextAttackDelay * GetAttackIntervalScale();
        phase2TeleportTimer_ = kPhase2TeleportMaximumInterval;
        phase2TeleportPositionIndex_ = 0;
        phase2TeleportTargetPositionIndex_ = 0;
        phase2TeleportEffectTimer_ = 0.0f;
        phase2TeleportState_ = Phase2TeleportState::Idle;
        phase2AttackEffectActiveCount_ = kPhase2AttackEffectCount;
        phase2AttackPattern_ = Phase2AttackPattern::EightWay;
        phase2SideSweepWaveIndex_ = 0;
        phase2SideSweepFirstDirection_ = 1.0f;
        isPhase2CloneActive_ = false;
        isPhase2ClonePending_ = false;
        isPhase2BossHiddenForClone_ = false;
        shouldSpawnPhase2CloneOnNextTeleport_ = true;
        isPhase1IntroActive_ = true;
        phase1IntroElapsedTime_ = 0.0f;
        previousSkyDomeUpdateTime_ = std::chrono::steady_clock::now();
        previousPlayerUpdateTime_ = std::chrono::steady_clock::now();
    }

    void GameScene::PrepareTutorial() {
        Reset();
        LoadStageForPhase(0);
        isTutorialMode_ = true;
        isPhase1IntroActive_ = false;
        phase1IntroElapsedTime_ = 0.0f;
        isPhase2IntroActive_ = false;
        phase2IntroElapsedTime_ = 0.0f;
        tutorialStep_ = TutorialStep::Move;
        tutorialStartPlayerX_ = playerPosition_.x;
        tutorialIconAnimationTime_ = 0.0f;
        tutorialAttackHintTimer_ = 0.0f;
        hasTutorialJumpStarted_ = false;
        suppressPlayerInputForOneFrame_ = true;

        // Keep one stationary enemy in the tutorial as a shooting target.
        enemyPositions_[0] = { 0.0f, 0.4f, 0.0f };
        enemyVerticalVelocities_[0] = 0.0f;
        enemyHealths_[0] = 100.0f;
        enemyBehaviors_[0] = EnemyBehavior::Stop;
        enemyBehaviorTimers_[0] = 0.0f;
        enemyMoveDirections_[0] = 0.0f;
        enemyMoveSpeeds_[0] = 0.0f;
        enemyTargetMoveSpeeds_[0] = 0.0f;
        enemySpeedVariationTimers_[0] = 0.0f;
        enemyFloatDirections_[0] = { 0.0f, 0.0f, 0.0f };
        enemyGrounded_[0] = false;
        enemyAttackCooldowns_[0] = 0.0f;
        enemyAttackHitPlayers_[0] = false;
        enemyRepeatDashUsed_[0] = false;
        enemyAttackDirections_[0] = { 0.0f, 0.0f, 0.0f };
    }

    bool GameScene::IsTutorialComplete() const {
        return tutorialStep_ == TutorialStep::Complete;
    }

    float GameScene::GetPhase1IntroCameraPullBackProgress() const {
        const float pullBackProgress = std::clamp(
            (phase1IntroElapsedTime_ - kPhase1IntroDescentDuration) /
                (kPhase1IntroDuration - kPhase1IntroDescentDuration),
            0.0f,
            1.0f);
        return pullBackProgress * pullBackProgress *
            (3.0f - 2.0f * pullBackProgress);
    }

    float GameScene::GetPhase2IntroCameraPullBackProgress() const {
        const float pullBackProgress = std::clamp(
            (phase2IntroElapsedTime_ - kPhase2IntroCameraReturnStartTime) /
                (kPhase2IntroDuration - kPhase2IntroCameraReturnStartTime),
            0.0f,
            1.0f);
        return pullBackProgress * pullBackProgress *
            (3.0f - 2.0f * pullBackProgress);
    }

    float GameScene::GetEnemyDefeatCameraPullBackProgress() const {
        if (bossPhase_ == 1) {
            const float zoomProgress = std::clamp(
                enemyDefeatAnimationTimer_ /
                    kPhase1DefeatCameraZoomDuration,
                0.0f,
                1.0f);
            const float easedZoom = zoomProgress * zoomProgress *
                (3.0f - 2.0f * zoomProgress);
            return 1.0f - easedZoom;
        }
        const float pullBackProgress = std::clamp(
            (enemyDefeatAnimationTimer_ -
                kPhase2DefeatCameraReturnStartTime) /
                (kPhase2DefeatAnimationDuration -
                    kPhase2DefeatCameraReturnStartTime),
            0.0f,
            1.0f);
        return pullBackProgress * pullBackProgress *
            (3.0f - 2.0f * pullBackProgress);
    }

    Vector3 GameScene::GetEnemyDefeatCameraTarget() const {
        if (enemyPositions_.empty()) {
            return { 0.0f, 0.0f, 2.134f };
        }
        if (bossPhase_ == 1) {
            const float returnProgress = std::clamp(
                (enemyDefeatAnimationTimer_ -
                    kPhase1TransformReturnStartTime) /
                    (kPhase1TransformReturnEndTime -
                        kPhase1TransformReturnStartTime),
                0.0f,
                1.0f);
            const float easedReturn = returnProgress * returnProgress *
                (3.0f - 2.0f * returnProgress);
            return {
                enemyPositions_[0].x * (1.0f - easedReturn),
                enemyPositions_[0].y * (1.0f - easedReturn),
                2.134f,
            };
        }
        return {
            enemyPositions_[0].x,
            enemyPositions_[0].y,
            2.134f,
        };
    }

    bool GameScene::AreAllEnemiesDefeated() const {
        if (isPhase1IntroActive_ || isPhase2IntroActive_) {
            return false;
        }
        return isEnemyDefeatAnimationFinished_ &&
            !enemyHealths_.empty() && std::all_of(
            enemyHealths_.begin(), enemyHealths_.end(),
            [](float health) { return health <= 0.0f; });
    }

    bool GameScene::TryAdvanceBossPhase() {
        if (!AreAllEnemiesDefeated() || bossPhase_ >= kBossPhaseCount) {
            return false;
        }

        return SetBossPhase(bossPhase_ + 1);
    }

    bool GameScene::SetBossPhase(int32_t phase) {
        const int32_t nextPhase = std::clamp(phase, 1, kBossPhaseCount);
        if (!LoadStageForPhase(nextPhase)) {
            return false;
        }
        bossPhase_ = nextPhase;

        // Keep the player state, but restart the boss encounter cleanly for
        // the selected phase.
        enemyPositions_ = { { 8.0f, -7.188f, 0.0f } };
        enemyVerticalVelocities_.assign(enemyPositions_.size(), 0.0f);
        enemyHealths_.assign(
            enemyPositions_.size(),
            nextPhase == 2 ? 300.0f : 100.0f);
        enemyBehaviors_.assign(enemyPositions_.size(), EnemyBehavior::Move);
        enemyBehaviorTimers_.clear();
        enemyBehaviorTimers_.reserve(enemyPositions_.size());
        for (size_t enemyIndex = 0; enemyIndex < enemyPositions_.size(); ++enemyIndex) {
            enemyBehaviorTimers_.push_back(GetRandomEnemyBehaviorDuration(EnemyBehavior::Move));
        }
        enemyMoveDirections_.assign(enemyPositions_.size(), -1.0f);
        enemyMoveSpeeds_.assign(enemyPositions_.size(), 0.0f);
        enemyTargetMoveSpeeds_.assign(enemyPositions_.size(), 0.0f);
        enemySpeedVariationTimers_.assign(enemyPositions_.size(), 0.0f);
        enemyFloatDirections_.clear();
        enemyFloatDirections_.reserve(enemyPositions_.size());
        for (size_t enemyIndex = 0; enemyIndex < enemyPositions_.size(); ++enemyIndex) {
            enemyFloatDirections_.push_back(GetRandomEnemyFloatDirection());
        }
        enemyGrounded_.assign(enemyPositions_.size(), false);
        enemyAttackCooldowns_.assign(enemyPositions_.size(), 0.0f);
        enemyAttackHitPlayers_.assign(enemyPositions_.size(), false);
        enemyRepeatDashUsed_.assign(enemyPositions_.size(), false);
        enemyAttackDirections_.assign(enemyPositions_.size(), { 1.0f, 0.0f, 0.0f });
        enemyAttackStartPositions_ = enemyPositions_;
        enemyContactDamageCooldown_ = 0.0f;
        playerInvincibilityTimer_ = 0.0f;
        playerHitFlashTimer_ = 0.0f;
        enemyHitFlashTimer_ = 0.0f;
        isEnemyDefeatAnimationActive_ = false;
        isEnemyDefeatAnimationFinished_ = false;
        enemyDefeatAnimationTimer_ = 0.0f;
        projectiles_.assign(kMaxProjectiles, {});
        arePhase2AttackEffectsActive_ = false;
        phase2AttackEffectRotation_ = 0.0f;
        phase2AttackEffectRotationDirection_ = 1.0f;
        phase2AttackEffectElapsedTime_ = 0.0f;
        phase2AttackDamageCooldown_ = 0.0f;
        phase2NextAttackTimer_ =
            kPhase2NextAttackDelay * GetAttackIntervalScale();
        phase2TeleportTimer_ = kPhase2TeleportMaximumInterval;
        phase2TeleportPositionIndex_ = 0;
        phase2TeleportTargetPositionIndex_ = 0;
        phase2TeleportEffectTimer_ = 0.0f;
        phase2TeleportState_ = Phase2TeleportState::Idle;
        phase2AttackEffectActiveCount_ = kPhase2AttackEffectCount;
        phase2AttackPattern_ = Phase2AttackPattern::EightWay;
        phase2SideSweepWaveIndex_ = 0;
        phase2SideSweepFirstDirection_ = 1.0f;
        isPhase2CloneActive_ = false;
        isPhase2ClonePending_ = false;
        isPhase2BossHiddenForClone_ = false;
        shouldSpawnPhase2CloneOnNextTeleport_ = true;
        isPhase1IntroActive_ = nextPhase == 1;
        phase1IntroElapsedTime_ = 0.0f;
        isPhase2IntroActive_ = nextPhase == 2;
        phase2IntroElapsedTime_ = 0.0f;
        if (isPhase1IntroActive_) {
            enemyPositions_[0] = { 0.0f, kPhase1IntroSpawnY, 0.0f };
            enemyHealths_[0] = 0.0f;
            enemyBehaviors_[0] = EnemyBehavior::Stop;
        }
        else if (isPhase2IntroActive_) {
            playerPosition_ = kPlayerEncounterStartPosition;
            playerHorizontalVelocity_ = 0.0f;
            playerVerticalVelocity_ = 0.0f;
            isPlayerGrounded_ = true;
            playerRotationY_ = kPlayerEncounterStartRotationY;
            playerTargetRotationY_ = playerRotationY_;
            phase2IntroStartPosition_ = { 0.0f, 0.0f, 0.0f };
            enemyPositions_[0] = phase2IntroStartPosition_;
            enemyHealths_[0] = 0.0f;
            enemyBehaviors_[0] = EnemyBehavior::Stop;
        }
        return true;
    }

    void GameScene::SpawnPhase2AttackEffects() {
        SpawnPhase2AttackPattern(Phase2AttackPattern::EightWay);
    }

    void GameScene::SpawnPhase2RainAttack() {
        SpawnPhase2AttackPattern(Phase2AttackPattern::Rain);
    }

    void GameScene::SpawnPhase2FastSpinAttack() {
        SpawnPhase2AttackPattern(Phase2AttackPattern::FastSpin);
    }

    void GameScene::SpawnPhase2SideSweepAttack() {
        SpawnPhase2AttackPattern(Phase2AttackPattern::SideSweep);
    }

    float GameScene::GetPhase2AttackTelegraphDuration() const {
        float duration = kPhase2RainTelegraphDuration;
        if (phase2AttackPattern_ == Phase2AttackPattern::EightWay) {
            duration = kPhase2EightWayTelegraphDuration;
        }
        else if (phase2AttackPattern_ == Phase2AttackPattern::FastSpin) {
            duration = kPhase2FastSpinTelegraphDuration;
        }
        else if (phase2AttackPattern_ == Phase2AttackPattern::SideSweep) {
            duration = kPhase2SideSweepTelegraphDuration;
        }
        if (isPhase2CloneActive_) {
            duration += kPhase2CloneTelegraphExtension;
        }
        return duration;
    }

    void GameScene::SpawnPhase2AttackPattern(
        Phase2AttackPattern pattern,
        bool isCloneAttack) {
        if (bossPhase_ != 2 || enemyPositions_.empty() || enemyHealths_[0] <= 0.0f) {
            return;
        }
        if (!isCloneAttack &&
            pattern == Phase2AttackPattern::EightWay &&
            phase2TeleportPositionIndex_ != 0) {
            return;
        }

        isPhase2CloneActive_ = isCloneAttack;
        phase2AttackPattern_ = pattern;
        phase2SideSweepWaveIndex_ = 0;
        phase2SideSweepFirstDirection_ = 1.0f;
        phase2AttackEffectRotation_ = 0.0f;
        std::bernoulli_distribution clockwiseDistribution(0.5);
        phase2AttackEffectRotationDirection_ =
            clockwiseDistribution(enemyRandomEngine_) ? -1.0f : 1.0f;
        phase2AttackEffectElapsedTime_ = 0.0f;
        phase2AttackDamageCooldown_ = 0.0f;
        phase2NextAttackTimer_ =
            kPhase2NextAttackDelay * GetAttackIntervalScale();

        if (pattern == Phase2AttackPattern::EightWay ||
            pattern == Phase2AttackPattern::FastSpin) {
            const Vector3& attackOrigin = isCloneAttack
                ? kPhase2ClonePosition
                : enemyPositions_[0];
            phase2AttackEffectActiveCount_ = kPhase2AttackEffectCount;
            const float spawnDistance = pattern == Phase2AttackPattern::FastSpin
                ? kPhase2FastSpinSpawnDistance
                : kPhase2AttackEffectSpawnDistance;
            constexpr float kDiagonal = 0.70710678f;
            constexpr std::array<Vector3, kPhase2AttackEffectCount> kDirections = {
                Vector3{  1.0f,       0.0f, 0.0f },
                Vector3{  kDiagonal,  kDiagonal, 0.0f },
                Vector3{  0.0f,       1.0f, 0.0f },
                Vector3{ -kDiagonal,  kDiagonal, 0.0f },
                Vector3{ -1.0f,       0.0f, 0.0f },
                Vector3{ -kDiagonal, -kDiagonal, 0.0f },
                Vector3{  0.0f,      -1.0f, 0.0f },
                Vector3{  kDiagonal, -kDiagonal, 0.0f },
            };
            for (size_t effectIndex = 0; effectIndex < phase2AttackEffectActiveCount_; ++effectIndex) {
                const Vector3& direction = kDirections[effectIndex];
                phase2AttackEffectDirections_[effectIndex] = direction;
                phase2AttackEffectPositions_[effectIndex] = {
                    attackOrigin.x + direction.x * spawnDistance,
                    attackOrigin.y + direction.y * spawnDistance,
                    attackOrigin.z,
                };
            }
        }
        else if (pattern == Phase2AttackPattern::Rain) {
            phase2AttackEffectActiveCount_ = kPhase2AttackEffectCount;
            std::uniform_real_distribution<float> rainXDistribution(-24.0f, 24.0f);
            for (size_t effectIndex = 0; effectIndex < phase2AttackEffectActiveCount_; ++effectIndex) {
                // Seven random strikes plus one strike locked to the player's
                // position at the beginning of the telegraph.
                const bool targetsPlayer = effectIndex == phase2AttackEffectActiveCount_ - 1;
                const float targetX = targetsPlayer
                    ? std::clamp(playerPosition_.x, -26.0f, 26.0f)
                    : rainXDistribution(enemyRandomEngine_);
                phase2AttackEffectDirections_[effectIndex] = { 0.0f, -1.0f, 0.0f };
                phase2AttackEffectPositions_[effectIndex] = { targetX, 0.0f, 0.0f };
            }
        }
        else {
            // One sweep is a four-lane set: three random heights and one lane
            // aimed at the player. The opposite side repeats the same set.
            phase2AttackEffectActiveCount_ = 4;
            std::uniform_real_distribution<float> sweepYDistribution(
                kPhase2SideSweepMinimumY,
                kPhase2SideSweepMaximumY);
            std::bernoulli_distribution directionDistribution(0.5);
            const float sweepDirection =
                directionDistribution(enemyRandomEngine_) ? 1.0f : -1.0f;
            phase2SideSweepFirstDirection_ = sweepDirection;
            for (size_t effectIndex = 0;
                effectIndex < phase2AttackEffectActiveCount_;
                ++effectIndex) {
                const bool targetsPlayer =
                    effectIndex == phase2AttackEffectActiveCount_ - 1;
                phase2AttackEffectDirections_[effectIndex] = {
                    sweepDirection,
                    0.0f,
                    0.0f,
                };
                phase2AttackEffectPositions_[effectIndex] = {
                    0.0f,
                    targetsPlayer
                        ? std::clamp(
                            playerPosition_.y,
                            kPhase2SideSweepMinimumY,
                            kPhase2SideSweepMaximumY)
                        : sweepYDistribution(enemyRandomEngine_),
                    0.0f,
                };
            }
        }
        arePhase2AttackEffectsActive_ = true;
    }

    bool GameScene::LoadStageForPhase(int32_t phase) {
        const char* stagePath = "MTEngine/Assets/Resources/Data/Stages/stage0.csv";
        if (phase == 1) {
            stagePath = "MTEngine/Assets/Resources/Data/Stages/stage1.csv";
        }
        else if (phase >= 2) {
            stagePath = "MTEngine/Assets/Resources/Data/Stages/stage2.csv";
        }

        TileMap nextTileMap;
        if (!nextTileMap.LoadFromCsv(stagePath)) {
            return false;
        }

        constexpr float kTileSize = 2.0f;
        const Vector3 mapOrigin = {
            -static_cast<float>(nextTileMap.GetWidth() - 1) * kTileSize * 0.5f,
            -static_cast<float>(nextTileMap.GetHeight() - 1) * kTileSize * 0.5f,
            0.0f,
        };
        std::vector<TileMapCell> nextCells = nextTileMap.CreateCells(kTileSize, mapOrigin);
        RemoveOpenBorderCells(nextCells, nextTileMap);

        while (transformConstantBuffers_.size() < nextCells.size()) {
            auto transformConstantBuffer = std::make_unique<TransformationMatrixConstantBuffer>();
            if (!device_ || !transformConstantBuffer->Initialize(device_)) {
                return false;
            }
            transformConstantBuffers_.push_back(std::move(transformConstantBuffer));
        }

        tileMap_ = std::move(nextTileMap);
        cells_ = std::move(nextCells);
        selectedCellIndex_ = -1;
        return true;
    }

    bool GameScene::IsPlayerDefeated() const {
        constexpr float kFallMissY = -30.0f;
        return isPlayerDeathAnimationFinished_ ||
            (playerHealth_ > 0.0f && playerPosition_.y < kFallMissY);
    }

    int32_t GameScene::GetRemainingProjectileCount() const {
        int32_t usedAmmo = 0;
        for (const Projectile& projectile : projectiles_) {
            if (projectile.isActive) {
                usedAmmo += projectile.ammoCost;
            }
        }
        return (std::max)(0, static_cast<int32_t>(kMaxProjectiles) - usedAmmo);
    }

    float GameScene::GetRandomEnemyBehaviorDuration(EnemyBehavior behavior) {
        float minimumDuration = 0.75f;
        float maximumDuration = 2.20f;
        if (behavior == EnemyBehavior::Stop) {
            minimumDuration = 0.35f;
            maximumDuration = 1.25f;
        }
        else if (behavior == EnemyBehavior::JumpPrepare) {
            minimumDuration = 0.12f;
            maximumDuration = 0.28f;
        }
        std::uniform_real_distribution<float> durationDistribution(minimumDuration, maximumDuration);
        return durationDistribution(enemyRandomEngine_);
    }

    float GameScene::GetRandomEnemyMoveSpeed() {
        std::uniform_real_distribution<float> speedDistribution(6.0f, 10.0f);
        return speedDistribution(enemyRandomEngine_);
    }

    float GameScene::GetRandomEnemySpeedVariationDuration() {
        std::uniform_real_distribution<float> durationDistribution(0.30f, 0.80f);
        return durationDistribution(enemyRandomEngine_);
    }

    Vector3 GameScene::GetRandomEnemyFloatDirection() {
        std::uniform_real_distribution<float> angleDistribution(
            0.0f,
            2.0f * std::numbers::pi_v<float>);
        const float angle = angleDistribution(enemyRandomEngine_);
        return { std::cos(angle), std::sin(angle), 0.0f };
    }

    float GameScene::GetRandomPhase2TeleportInterval() {
        std::uniform_real_distribution<float> intervalDistribution(
            kPhase2TeleportMinimumInterval,
            kPhase2TeleportMaximumInterval);
        return intervalDistribution(enemyRandomEngine_);
    }

    void GameScene::StartEnemyDefeatAnimation() {
        if (isTutorialMode_ || isEnemyDefeatAnimationActive_ ||
            isEnemyDefeatAnimationFinished_) {
            return;
        }

        isEnemyDefeatAnimationActive_ = true;
        enemyDefeatAnimationTimer_ = 0.0f;
        playerHorizontalVelocity_ = 0.0f;
        playerVerticalVelocity_ = 0.0f;
        playerChargeTime_ = 0.0f;
        isPlayerCharging_ = false;
        projectiles_.assign(kMaxProjectiles, {});
        arePhase2AttackEffectsActive_ = false;
        isPhase2CloneActive_ = false;
        isPhase2ClonePending_ = false;
        isPhase2BossHiddenForClone_ = false;
        phase2TeleportState_ = Phase2TeleportState::Idle;
        phase2TeleportEffectTimer_ = 0.0f;
    }

    void GameScene::UpdateSkyDome() {
        const auto now = std::chrono::steady_clock::now();
        if (previousSkyDomeUpdateTime_ ==
            std::chrono::steady_clock::time_point{}) {
            previousSkyDomeUpdateTime_ = now;
            return;
        }

        const float deltaTime = (std::min)(
            std::chrono::duration<float>(
                now - previousSkyDomeUpdateTime_).count(),
            1.0f / 30.0f);
        previousSkyDomeUpdateTime_ = now;
        skyDomeRotationY_ = std::fmod(
            skyDomeRotationY_ + kSkyDomeRotationSpeed * deltaTime,
            2.0f * std::numbers::pi_v<float>);
        titleAnimationTime_ = std::fmod(
            titleAnimationTime_ + deltaTime,
            2.0f * std::numbers::pi_v<float>);
    }

    void GameScene::UpdatePlayer(const Input* input) {
        if (previousPlayerUpdateTime_ == std::chrono::steady_clock::time_point{}) {
            previousPlayerUpdateTime_ = std::chrono::steady_clock::now();
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        const float deltaTime = (std::min)(
            std::chrono::duration<float>(now - previousPlayerUpdateTime_).count(),
            1.0f / 30.0f);
        previousPlayerUpdateTime_ = now;
        playerInvincibilityTimer_ =
            (std::max)(0.0f, playerInvincibilityTimer_ - deltaTime);
        playerHitFlashTimer_ = (std::max)(0.0f, playerHitFlashTimer_ - deltaTime);
        enemyHitFlashTimer_ = (std::max)(0.0f, enemyHitFlashTimer_ - deltaTime);

        if (isPhase1IntroActive_) {
            phase1IntroElapsedTime_ = (std::min)(
                phase1IntroElapsedTime_ + deltaTime,
                kPhase1IntroDuration);

            const float descentProgress = std::clamp(
                phase1IntroElapsedTime_ / kPhase1IntroDescentDuration,
                0.0f,
                1.0f);
            const float easedDescentProgress = descentProgress * descentProgress *
                (3.0f - 2.0f * descentProgress);
            if (!enemyPositions_.empty()) {
                enemyPositions_[0] = {
                    0.0f,
                    kPhase1IntroSpawnY +
                        (kPhase1IntroLandingY - kPhase1IntroSpawnY) *
                            easedDescentProgress,
                    0.0f,
                };
                const float healthFillProgress = std::clamp(
                    (phase1IntroElapsedTime_ - kPhase1IntroHealthFillStartTime) /
                        (kPhase1IntroDuration - kPhase1IntroHealthFillStartTime),
                    0.0f,
                    1.0f);
                enemyHealths_[0] = GetEnemyMaxHealth() * healthFillProgress;
            }

            playerHorizontalVelocity_ = 0.0f;
            playerVerticalVelocity_ = 0.0f;
            playerChargeTime_ = 0.0f;
            isPlayerCharging_ = false;

            if (phase1IntroElapsedTime_ >= kPhase1IntroDuration) {
                isPhase1IntroActive_ = false;
                if (!enemyPositions_.empty()) {
                    enemyPositions_[0] = { 0.0f, kPhase1IntroLandingY, 0.0f };
                    enemyHealths_[0] = GetEnemyMaxHealth();
                    enemyBehaviors_[0] = EnemyBehavior::Move;
                    enemyBehaviorTimers_[0] =
                        GetRandomEnemyBehaviorDuration(EnemyBehavior::Move);
                }
            }
            return;
        }

        if (isPhase2IntroActive_) {
            phase2IntroElapsedTime_ = (std::min)(
                phase2IntroElapsedTime_ + deltaTime,
                kPhase2IntroDuration);

            if (!enemyPositions_.empty()) {
                enemyPositions_[0] = phase2IntroStartPosition_;
                const float healthFillProgress = std::clamp(
                    (phase2IntroElapsedTime_ -
                        kPhase2IntroHealthFillStartTime) /
                        (kPhase2IntroDuration -
                            kPhase2IntroHealthFillStartTime),
                    0.0f,
                    1.0f);
                const float easedHealthFill =
                    healthFillProgress * healthFillProgress *
                    (3.0f - 2.0f * healthFillProgress);
                enemyHealths_[0] = GetEnemyMaxHealth() * easedHealthFill;
            }

            playerHorizontalVelocity_ = 0.0f;
            playerVerticalVelocity_ = 0.0f;
            playerChargeTime_ = 0.0f;
            isPlayerCharging_ = false;

            if (phase2IntroElapsedTime_ >= kPhase2IntroDuration) {
                isPhase2IntroActive_ = false;
                if (!enemyPositions_.empty()) {
                    enemyPositions_[0] = kPhase2TeleportPositions[0];
                    enemyHealths_[0] = GetEnemyMaxHealth();
                    enemyBehaviors_[0] = EnemyBehavior::Move;
                    enemyBehaviorTimers_[0] =
                        GetRandomEnemyBehaviorDuration(EnemyBehavior::Move);
                }
            }
            return;
        }

        const bool areEnemiesAtZeroHealth =
            !enemyHealths_.empty() && std::all_of(
                enemyHealths_.begin(),
                enemyHealths_.end(),
                [](float health) { return health <= 0.0f; });
        if (areEnemiesAtZeroHealth &&
            !isEnemyDefeatAnimationActive_ &&
            !isEnemyDefeatAnimationFinished_) {
            StartEnemyDefeatAnimation();
        }
        if (isEnemyDefeatAnimationActive_) {
            const float animationDuration = bossPhase_ == 1
                ? kPhase1DefeatAnimationDuration
                : kPhase2DefeatAnimationDuration;
            const float previousDefeatAnimationTimer =
                enemyDefeatAnimationTimer_;
            enemyDefeatAnimationTimer_ = (std::min)(
                enemyDefeatAnimationTimer_ + deltaTime,
                animationDuration);
            if (bossPhase_ == 2 &&
                previousDefeatAnimationTimer < kPhase2DefeatDisappearTime &&
                enemyDefeatAnimationTimer_ >= kPhase2DefeatDisappearTime) {
                teleportSoundRequested_ = true;
            }
            playerHorizontalVelocity_ = 0.0f;
            playerVerticalVelocity_ = 0.0f;
            playerChargeTime_ = 0.0f;
            isPlayerCharging_ = false;
            if (enemyDefeatAnimationTimer_ >= animationDuration) {
                isEnemyDefeatAnimationActive_ = false;
                isEnemyDefeatAnimationFinished_ = true;
            }
            return;
        }

        if (playerHealth_ <= 0.0f &&
            !isPlayerDeathAnimationActive_ &&
            !isPlayerDeathAnimationFinished_) {
            isPlayerDeathAnimationActive_ = true;
            playerDeathAnimationTimer_ = 0.0f;
            playerDeathRotationX_ = 0.0f;
            playerHorizontalVelocity_ = 0.0f;
            playerVerticalVelocity_ = kPlayerDeathInitialUpwardSpeed;
            isPlayerGrounded_ = false;
            playerChargeTime_ = 0.0f;
            isPlayerCharging_ = false;
            playerInvincibilityTimer_ = 0.0f;
            playerHitFlashTimer_ = 0.0f;
        }

        if (isPlayerDeathAnimationActive_) {
            playerDeathAnimationTimer_ += deltaTime;
            playerVerticalVelocity_ += kPlayerDeathGravity * deltaTime;
            playerPosition_.y += playerVerticalVelocity_ * deltaTime;
            playerDeathRotationX_ = (std::min)(
                std::numbers::pi_v<float> / 2.0f,
                playerDeathRotationX_ + kPlayerDeathRotationSpeed * deltaTime);
            if (playerDeathAnimationTimer_ >= kPlayerDeathAnimationDuration) {
                isPlayerDeathAnimationActive_ = false;
                isPlayerDeathAnimationFinished_ = true;
            }
            return;
        }

        constexpr float kGravity = -34.0f;
        constexpr float kPlayerHalfWidth = 0.6f;
        constexpr float kPlayerHalfHeight = 0.8f;
        constexpr float kTileHalfExtent = 1.0f;
        constexpr float kTileTopOffset = 1.0f;
        constexpr float kPlayerTurnResponse = 30.0f;

        float horizontalInput = 0.0f;
        bool jumpRequested = false;
        bool attackHeld = false;
        bool attackReleased = false;
        const bool shouldSuppressPlayerInput =
            suppressPlayerInputForOneFrame_;
        suppressPlayerInputForOneFrame_ = false;
        if (input && !shouldSuppressPlayerInput) {
            if (input->PushKey(DIK_A) || input->PushKey(DIK_LEFT)) {
                horizontalInput -= 1.0f;
            }
            if (input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT)) {
                horizontalInput += 1.0f;
            }
            jumpRequested = input->TriggerKey(DIK_W);
            attackHeld = input->PushKey(DIK_SPACE);
            attackReleased = input->ExitKey(DIK_SPACE);

            const GamePad* gamePad = input->GetGamePad();
            if (gamePad && gamePad->IsConnected()) {
                horizontalInput += gamePad->GetLeftStick().x;
                jumpRequested = jumpRequested || gamePad->TriggerButton(GamePadButton::A);
                attackHeld = attackHeld || gamePad->PushRightTrigger();
                attackReleased = attackReleased || gamePad->ExitRightTrigger();
            }

            horizontalInput = std::clamp(horizontalInput, -1.0f, 1.0f);
            if (isPlayerGrounded_ && jumpRequested) {
                playerVerticalVelocity_ = playerJumpSpeed_;
                isPlayerGrounded_ = false;
            }
        }

        if (horizontalInput != 0.0f) {
            playerHorizontalVelocity_ += horizontalInput * playerMoveAcceleration_ * deltaTime;
        }
        else {
            playerHorizontalVelocity_ *= std::exp(-playerVelocityDamping_ * deltaTime);
            if (std::abs(playerHorizontalVelocity_) < 0.01f) {
                playerHorizontalVelocity_ = 0.0f;
            }
        }
        playerHorizontalVelocity_ = std::clamp(
            playerHorizontalVelocity_,
            -playerMaxMoveSpeed_,
            playerMaxMoveSpeed_);

        // Turn toward the requested direction independently of movement momentum.
        // This reaches almost the target angle in about 0.1 seconds.
        if (std::abs(horizontalInput) >= 0.01f) {
            playerTargetRotationY_ = horizontalInput > 0.0f
                ? -std::numbers::pi_v<float> / 2.0f
                : std::numbers::pi_v<float> / 2.0f;
            const float rotationDifference = std::remainder(
                playerTargetRotationY_ - playerRotationY_,
                2.0f * std::numbers::pi_v<float>);
            const float interpolation = 1.0f - std::exp(-kPlayerTurnResponse * deltaTime);
            playerRotationY_ = std::remainder(
                playerRotationY_ + rotationDifference * interpolation,
                2.0f * std::numbers::pi_v<float>);
        }

        const float previousX = playerPosition_.x;
        playerPosition_.x += playerHorizontalVelocity_ * deltaTime;

        const float playerBottom = playerPosition_.y - kPlayerHalfHeight;
        const float playerTop = playerPosition_.y + kPlayerHalfHeight;
        for (const TileMapCell& cell : cells_) {
            const float tileBottom = cell.worldPosition.y - kTileHalfExtent;
            const float tileTop = cell.worldPosition.y + kTileHalfExtent;
            if (playerBottom >= tileTop || playerTop <= tileBottom) {
                continue;
            }

            const float tileLeft = cell.worldPosition.x - kTileHalfExtent;
            const float tileRight = cell.worldPosition.x + kTileHalfExtent;
            const float previousLeft = previousX - kPlayerHalfWidth;
            const float previousRight = previousX + kPlayerHalfWidth;
            const float playerLeft = playerPosition_.x - kPlayerHalfWidth;
            const float playerRight = playerPosition_.x + kPlayerHalfWidth;

            if (playerHorizontalVelocity_ < 0.0f && previousLeft >= tileRight && playerLeft < tileRight) {
                playerPosition_.x = (std::max)(playerPosition_.x, tileRight + kPlayerHalfWidth);
                playerHorizontalVelocity_ = 0.0f;
            }
            else if (playerHorizontalVelocity_ > 0.0f && previousRight <= tileLeft && playerRight > tileLeft) {
                playerPosition_.x = (std::min)(playerPosition_.x, tileLeft - kPlayerHalfWidth);
                playerHorizontalVelocity_ = 0.0f;
            }
        }

        // The left and right border tiles are intentionally invisible. Keep
        // the player inside the unchanged map dimensions with a logical bound.
        const float mapHalfWidth = static_cast<float>(tileMap_.GetWidth());
        const float minimumPlayerX = -mapHalfWidth + kPlayerHalfWidth;
        const float maximumPlayerX = mapHalfWidth - kPlayerHalfWidth;
        const float boundedPlayerX = std::clamp(
            playerPosition_.x,
            minimumPlayerX,
            maximumPlayerX);
        if (boundedPlayerX != playerPosition_.x) {
            playerPosition_.x = boundedPlayerX;
            playerHorizontalVelocity_ = 0.0f;
        }

        playerVerticalVelocity_ += kGravity * deltaTime;
        const float previousY = playerPosition_.y;
        playerPosition_.y += playerVerticalVelocity_ * deltaTime;

        isPlayerGrounded_ = false;
        float landingY = -FLT_MAX;
        for (const TileMapCell& cell : cells_) {
            if (std::abs(playerPosition_.x - cell.worldPosition.x) > kTileHalfExtent + kPlayerHalfWidth) {
                continue;
            }

            const float tileTop = cell.worldPosition.y + kTileTopOffset;
            const float playerBottomBeforeMove = previousY - kPlayerHalfHeight;
            const float playerBottomAfterMove = playerPosition_.y - kPlayerHalfHeight;
            if (playerVerticalVelocity_ <= 0.0f &&
                playerBottomBeforeMove >= tileTop &&
                playerBottomAfterMove <= tileTop) {
                landingY = (std::max)(landingY, tileTop + kPlayerHalfHeight);
            }
        }

        if (landingY > -FLT_MAX) {
            playerPosition_.y = landingY;
            playerVerticalVelocity_ = 0.0f;
            isPlayerGrounded_ = true;
        }
        else if (playerVerticalVelocity_ > 0.0f) {
            float ceilingY = FLT_MAX;
            for (const TileMapCell& cell : cells_) {
                if (std::abs(playerPosition_.x - cell.worldPosition.x) > kTileHalfExtent + kPlayerHalfWidth) {
                    continue;
                }

                const float tileBottom = cell.worldPosition.y - kTileTopOffset;
                const float playerTopBeforeMove = previousY + kPlayerHalfHeight;
                const float playerTopAfterMove = playerPosition_.y + kPlayerHalfHeight;
                if (playerTopBeforeMove <= tileBottom &&
                    playerTopAfterMove >= tileBottom) {
                    ceilingY = (std::min)(ceilingY, tileBottom);
                }
            }

            if (ceilingY < FLT_MAX) {
                playerPosition_.y = ceilingY - kPlayerHalfHeight;
                playerVerticalVelocity_ = 0.0f;
            }
        }

        // The top border is also invisible, so stop upward movement at the
        // logical edge of the existing 14-row map.
        const float mapHalfHeight = static_cast<float>(tileMap_.GetHeight());
        const float maximumPlayerY = mapHalfHeight - kPlayerHalfHeight;
        if (playerPosition_.y > maximumPlayerY) {
            playerPosition_.y = maximumPlayerY;
            playerVerticalVelocity_ = 0.0f;
        }

        // Flying enemies orbit the player while correcting toward a preferred
        // distance. Their X and Y movement are resolved against the outer wall.
        constexpr float kEnemyHalfWidth = 0.95f;
        constexpr float kEnemyBottomOffset = 0.812f;
        constexpr float kEnemyTopOffset = 1.20f;
        constexpr float kEnemyMoveAcceleration = 30.0f;
        constexpr float kEnemyOrbitRadius = 9.0f;
        constexpr float kEnemyOrbitCorrectionDistance = 2.0f;
        constexpr float kEnemyAttackDetectionRange = 10.0f;
        constexpr float kEnemyAttackPrepareDuration = 0.25f;
        constexpr float kEnemyAttackDuration = 0.35f;
        constexpr float kEnemyAttackSpeed = 25.0f;
        constexpr float kEnemyAttackCooldown = 2.5f;
        constexpr float kPhase1RepeatDashProbability = 0.50f;

        const auto isOuterMapCell = [&](const TileMapCell& cell) {
            return cell.column == 0 ||
                cell.column == tileMap_.GetWidth() - 1 ||
                cell.row == 0 ||
                cell.row == tileMap_.GetHeight() - 1;
        };

        for (size_t enemyIndex = 0; enemyIndex < enemyPositions_.size(); ++enemyIndex) {
            if (enemyHealths_[enemyIndex] <= 0.0f) {
                continue;
            }

            if (isTutorialMode_) {
                enemyVerticalVelocities_[enemyIndex] = 0.0f;
                enemyMoveSpeeds_[enemyIndex] = 0.0f;
                enemyTargetMoveSpeeds_[enemyIndex] = 0.0f;
                continue;
            }

            if (bossPhase_ == 2) {
                constexpr float kPhase2TeleportHealthThreshold = 200.0f;
                if (enemyHealths_[enemyIndex] >= kPhase2TeleportHealthThreshold) {
                    enemyPositions_[enemyIndex] = kPhase2TeleportPositions[0];
                    phase2TeleportTimer_ = kPhase2TeleportMaximumInterval;
                    phase2TeleportPositionIndex_ = 0;
                    phase2TeleportTargetPositionIndex_ = 0;
                    phase2TeleportEffectTimer_ = 0.0f;
                    phase2TeleportState_ = Phase2TeleportState::Idle;
                    isPhase2ClonePending_ = false;
                    isPhase2BossHiddenForClone_ = false;
                }
                else {
                    phase2TeleportTimer_ -= deltaTime;
                    if (phase2TeleportState_ != Phase2TeleportState::Idle) {
                        phase2TeleportEffectTimer_ = (std::max)(
                            0.0f,
                            phase2TeleportEffectTimer_ - deltaTime);
                        if (phase2TeleportEffectTimer_ <= 0.0f) {
                            if (phase2TeleportState_ ==
                                Phase2TeleportState::Disappearing) {
                                phase2TeleportPositionIndex_ =
                                    phase2TeleportTargetPositionIndex_;
                                enemyPositions_[enemyIndex] =
                                    kPhase2TeleportPositions[
                                        phase2TeleportPositionIndex_];
                                if (isPhase2ClonePending_) {
                                    // Keep the real boss hidden at its destination
                                    // until the clone finishes its single attack.
                                    isPhase2BossHiddenForClone_ = true;
                                    phase2TeleportState_ =
                                        Phase2TeleportState::Idle;
                                    phase2TeleportEffectTimer_ = 0.0f;
                                    isPhase2ClonePending_ = false;
                                    SpawnPhase2AttackPattern(
                                        pendingPhase2CloneAttackPattern_,
                                        true);
                                }
                                else {
                                    phase2TeleportState_ =
                                        Phase2TeleportState::Appearing;
                                    phase2TeleportEffectTimer_ =
                                        kPhase2TeleportEffectDuration;
                                }
                            }
                            else {
                                phase2TeleportState_ =
                                    Phase2TeleportState::Idle;
                            }
                        }
                    }
                    else if (phase2TeleportTimer_ <= 0.0f &&
                        !arePhase2AttackEffectsActive_) {
                        const bool isClonePhase =
                            enemyHealths_[enemyIndex] <=
                                kPhase2CloneHealthThreshold;
                        const bool shouldSpawnClone =
                            isClonePhase &&
                            shouldSpawnPhase2CloneOnNextTeleport_;
                        if (isClonePhase) {
                            shouldSpawnPhase2CloneOnNextTeleport_ =
                                !shouldSpawnPhase2CloneOnNextTeleport_;
                        }
                        if (shouldSpawnClone) {
                            std::uniform_int_distribution<size_t>
                                clonePhaseTargetDistribution(
                                    1,
                                    kPhase2TeleportPositions.size() - 1);
                            do {
                                phase2TeleportTargetPositionIndex_ =
                                    clonePhaseTargetDistribution(
                                        enemyRandomEngine_);
                            } while (phase2TeleportTargetPositionIndex_ ==
                                phase2TeleportPositionIndex_);
                        }
                        else {
                            std::uniform_int_distribution<size_t>
                                offsetDistribution(
                                    1,
                                    kPhase2TeleportPositions.size() - 1);
                            phase2TeleportTargetPositionIndex_ =
                                (phase2TeleportPositionIndex_ +
                                    offsetDistribution(enemyRandomEngine_)) %
                                kPhase2TeleportPositions.size();
                        }
                        phase2TeleportState_ =
                            Phase2TeleportState::Disappearing;
                        teleportSoundRequested_ = true;
                        phase2TeleportEffectTimer_ =
                            kPhase2TeleportEffectDuration;
                        phase2TeleportTimer_ = GetRandomPhase2TeleportInterval();
                        if (shouldSpawnClone) {
                            isPhase2BossHiddenForClone_ = false;
                            std::uniform_int_distribution<int32_t>
                                cloneAttackDistribution(0, 3);
                            pendingPhase2CloneAttackPattern_ =
                                static_cast<Phase2AttackPattern>(
                                    cloneAttackDistribution(
                                        enemyRandomEngine_));
                            isPhase2ClonePending_ = true;
                        }
                        else {
                            isPhase2ClonePending_ = false;
                        }
                    }
                }
                enemyVerticalVelocities_[enemyIndex] = 0.0f;
                enemyMoveSpeeds_[enemyIndex] = 0.0f;
                enemyTargetMoveSpeeds_[enemyIndex] = 0.0f;
                continue;
            }

            Vector3& enemyPosition = enemyPositions_[enemyIndex];
            float& enemyVerticalVelocity = enemyVerticalVelocities_[enemyIndex];
            EnemyBehavior& enemyBehavior = enemyBehaviors_[enemyIndex];
            float& behaviorTimer = enemyBehaviorTimers_[enemyIndex];
            float& moveDirection = enemyMoveDirections_[enemyIndex];
            float& moveSpeed = enemyMoveSpeeds_[enemyIndex];
            float& targetMoveSpeed = enemyTargetMoveSpeeds_[enemyIndex];
            float& speedVariationTimer = enemySpeedVariationTimers_[enemyIndex];
            Vector3& floatDirection = enemyFloatDirections_[enemyIndex];
            float& attackCooldown = enemyAttackCooldowns_[enemyIndex];
            Vector3& attackDirection = enemyAttackDirections_[enemyIndex];

            attackCooldown = (std::max)(0.0f, attackCooldown - deltaTime);
            const Vector3 toPlayer = {
                playerPosition_.x - enemyPosition.x,
                playerPosition_.y - enemyPosition.y,
                0.0f,
            };
            const float playerDistance = std::sqrt(
                toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);
            const auto tryStartRepeatDash = [&]() {
                if (enemyBehavior != EnemyBehavior::Attack ||
                    enemyRepeatDashUsed_[enemyIndex] ||
                    enemyHealths_[enemyIndex] >= GetEnemyMaxHealth() * 0.5f ||
                    playerDistance <= 0.01f) {
                    return false;
                }

                std::uniform_real_distribution<float> probabilityDistribution(
                    0.0f,
                    1.0f);
                if (probabilityDistribution(enemyRandomEngine_) >=
                    kPhase1RepeatDashProbability) {
                    return false;
                }

                attackDirection = {
                    toPlayer.x / playerDistance,
                    toPlayer.y / playerDistance,
                    0.0f,
                };
                enemyBehavior = EnemyBehavior::AttackPrepare;
                behaviorTimer = kEnemyAttackPrepareDuration;
                moveSpeed = 0.0f;
                targetMoveSpeed = 0.0f;
                enemyRepeatDashUsed_[enemyIndex] = true;
                return true;
            };
            const bool canStartAttack =
                enemyBehavior != EnemyBehavior::Attack &&
                enemyBehavior != EnemyBehavior::AttackPrepare &&
                attackCooldown <= 0.0f &&
                playerDistance > 0.01f &&
                playerDistance <= kEnemyAttackDetectionRange;
            if (canStartAttack) {
                attackDirection = {
                    toPlayer.x / playerDistance,
                    toPlayer.y / playerDistance,
                    toPlayer.z / playerDistance,
                };
                enemyBehavior = EnemyBehavior::AttackPrepare;
                behaviorTimer = kEnemyAttackPrepareDuration;
                moveSpeed = 0.0f;
                targetMoveSpeed = 0.0f;
                enemyRepeatDashUsed_[enemyIndex] = false;
            }

            behaviorTimer -= deltaTime;
            if (behaviorTimer <= 0.0f) {
                if (enemyBehavior == EnemyBehavior::Move ||
                    enemyBehavior == EnemyBehavior::Stop ||
                    enemyBehavior == EnemyBehavior::JumpPrepare) {
                    std::uniform_real_distribution<float> probabilityDistribution(0.0f, 1.0f);
                    if (probabilityDistribution(enemyRandomEngine_) < 0.20f) {
                        moveDirection = -moveDirection;
                    }
                    enemyBehavior = EnemyBehavior::Move;
                    behaviorTimer = GetRandomEnemyBehaviorDuration(enemyBehavior);
                    targetMoveSpeed = GetRandomEnemyMoveSpeed();
                    speedVariationTimer = GetRandomEnemySpeedVariationDuration();
                }
                else if (enemyBehavior == EnemyBehavior::AttackPrepare) {
                    enemyBehavior = EnemyBehavior::Attack;
                    dashSoundRequested_ = true;
                    behaviorTimer = kEnemyAttackDuration;
                    targetMoveSpeed = kEnemyAttackSpeed;
                    enemyAttackHitPlayers_[enemyIndex] = false;
                    enemyAttackStartPositions_[enemyIndex] = enemyPosition;
                    // A new dash should not be suppressed by a prior touch hit.
                    enemyContactDamageCooldown_ = 0.0f;
                }
                else {
                    if (!tryStartRepeatDash()) {
                        enemyBehavior = EnemyBehavior::Move;
                        behaviorTimer = GetRandomEnemyBehaviorDuration(enemyBehavior);
                        moveSpeed = 0.0f;
                        targetMoveSpeed = GetRandomEnemyMoveSpeed();
                        floatDirection = GetRandomEnemyFloatDirection();
                        speedVariationTimer = GetRandomEnemySpeedVariationDuration();
                        attackCooldown =
                            kEnemyAttackCooldown * GetAttackIntervalScale();
                        enemyRepeatDashUsed_[enemyIndex] = false;
                    }
                }
            }

            Vector3 intendedMovement{};
            if (enemyBehavior == EnemyBehavior::Move) {
                speedVariationTimer -= deltaTime;
                if (speedVariationTimer <= 0.0f) {
                    targetMoveSpeed = GetRandomEnemyMoveSpeed();
                    speedVariationTimer = GetRandomEnemySpeedVariationDuration();
                }
                moveSpeed = (std::min)(targetMoveSpeed, moveSpeed + kEnemyMoveAcceleration * deltaTime);

                if (playerDistance > 0.01f) {
                    const Vector3 outwardDirection = {
                        -toPlayer.x / playerDistance,
                        -toPlayer.y / playerDistance,
                        0.0f,
                    };
                    const Vector3 tangentDirection = {
                        -outwardDirection.y * moveDirection,
                        outwardDirection.x * moveDirection,
                        0.0f,
                    };
                    const float radialCorrection = std::clamp(
                        (playerDistance - kEnemyOrbitRadius) / kEnemyOrbitCorrectionDistance,
                        -2.0f,
                        2.0f);
                    const Vector3 desiredDirection = {
                        tangentDirection.x - outwardDirection.x * radialCorrection,
                        tangentDirection.y - outwardDirection.y * radialCorrection,
                        0.0f,
                    };
                    const float desiredLength = std::sqrt(
                        desiredDirection.x * desiredDirection.x +
                        desiredDirection.y * desiredDirection.y);
                    if (desiredLength > 0.01f) {
                        floatDirection = {
                            desiredDirection.x / desiredLength,
                            desiredDirection.y / desiredLength,
                            0.0f,
                        };
                    }
                }

                intendedMovement.x = floatDirection.x * moveSpeed * deltaTime;
                intendedMovement.y = floatDirection.y * moveSpeed * deltaTime;
            }
            else if (enemyBehavior == EnemyBehavior::Attack) {
                moveSpeed = kEnemyAttackSpeed;
                intendedMovement.x = attackDirection.x * moveSpeed * deltaTime;
                intendedMovement.y = attackDirection.y * moveSpeed * deltaTime;
            }

            bool hitBlock = false;
            enemyPosition.x += intendedMovement.x;
            // Resolve X before applying Y so diagonal movement cannot choose
            // the wrong collision side at a tile corner.
            for (const TileMapCell& cell : cells_) {
                if (!isOuterMapCell(cell)) {
                    continue;
                }

                const float tileBottom = cell.worldPosition.y - kTileHalfExtent;
                const float tileTop = cell.worldPosition.y + kTileHalfExtent;
                const float enemyBottom = enemyPosition.y - kEnemyBottomOffset;
                const float enemyTop = enemyPosition.y + kEnemyTopOffset;
                if (enemyBottom >= tileTop || enemyTop <= tileBottom) {
                    continue;
                }

                const float tileLeft = cell.worldPosition.x - kTileHalfExtent;
                const float tileRight = cell.worldPosition.x + kTileHalfExtent;
                const float enemyLeft = enemyPosition.x - kEnemyHalfWidth;
                const float enemyRight = enemyPosition.x + kEnemyHalfWidth;
                if (enemyRight <= tileLeft || enemyLeft >= tileRight) {
                    continue;
                }

                const float horizontalMovement = intendedMovement.x;
                if (horizontalMovement > 0.0f) {
                    enemyPosition.x = tileLeft - kEnemyHalfWidth;
                }
                else if (horizontalMovement < 0.0f) {
                    enemyPosition.x = tileRight + kEnemyHalfWidth;
                }
                else {
                    const float positionOnLeft = tileLeft - kEnemyHalfWidth;
                    const float positionOnRight = tileRight + kEnemyHalfWidth;
                    enemyPosition.x = std::abs(enemyPosition.x - positionOnLeft) <
                        std::abs(enemyPosition.x - positionOnRight)
                        ? positionOnLeft
                        : positionOnRight;
                }
                hitBlock = true;
            }
            const float minimumEnemyX = -mapHalfWidth + kEnemyHalfWidth;
            const float maximumEnemyX = mapHalfWidth - kEnemyHalfWidth;
            const float boundedEnemyX = std::clamp(
                enemyPosition.x,
                minimumEnemyX,
                maximumEnemyX);
            if (boundedEnemyX != enemyPosition.x) {
                enemyPosition.x = boundedEnemyX;
                hitBlock = true;
            }

            enemyPosition.y += intendedMovement.y;
            // Resolve Y using the horizontally corrected position.
            for (const TileMapCell& cell : cells_) {
                if (!isOuterMapCell(cell)) {
                    continue;
                }

                const float tileLeft = cell.worldPosition.x - kTileHalfExtent;
                const float tileRight = cell.worldPosition.x + kTileHalfExtent;
                const float enemyLeft = enemyPosition.x - kEnemyHalfWidth;
                const float enemyRight = enemyPosition.x + kEnemyHalfWidth;
                if (enemyRight <= tileLeft || enemyLeft >= tileRight) {
                    continue;
                }

                const float tileBottom = cell.worldPosition.y - kTileHalfExtent;
                const float tileTop = cell.worldPosition.y + kTileTopOffset;
                const float enemyBottom = enemyPosition.y - kEnemyBottomOffset;
                const float enemyTop = enemyPosition.y + kEnemyTopOffset;
                if (enemyBottom >= tileTop || enemyTop <= tileBottom) {
                    continue;
                }

                const float verticalMovement = intendedMovement.y;
                if (verticalMovement > 0.0f) {
                    enemyPosition.y = tileBottom - kEnemyTopOffset;
                }
                else if (verticalMovement < 0.0f) {
                    enemyPosition.y = tileTop + kEnemyBottomOffset;
                }
                else {
                    const float positionBelow = tileBottom - kEnemyTopOffset;
                    const float positionAbove = tileTop + kEnemyBottomOffset;
                    enemyPosition.y = std::abs(enemyPosition.y - positionBelow) <
                        std::abs(enemyPosition.y - positionAbove)
                        ? positionBelow
                        : positionAbove;
                }
                hitBlock = true;
            }
            const float minimumEnemyY =
                -mapHalfHeight + kEnemyBottomOffset;
            const float maximumEnemyY =
                mapHalfHeight - kEnemyTopOffset;
            const float boundedEnemyY = std::clamp(
                enemyPosition.y,
                minimumEnemyY,
                maximumEnemyY);
            if (boundedEnemyY != enemyPosition.y) {
                enemyPosition.y = boundedEnemyY;
                hitBlock = true;
            }

            if (hitBlock) {
                const bool startedRepeatDash =
                    enemyBehavior == EnemyBehavior::Attack &&
                    tryStartRepeatDash();
                if (!startedRepeatDash) {
                    if (enemyBehavior == EnemyBehavior::Attack) {
                        attackCooldown =
                            kEnemyAttackCooldown * GetAttackIntervalScale();
                    }
                    moveSpeed = 0.0f;
                    targetMoveSpeed = GetRandomEnemyMoveSpeed();
                    moveDirection = -moveDirection;
                    enemyBehavior = EnemyBehavior::Move;
                    behaviorTimer = GetRandomEnemyBehaviorDuration(enemyBehavior);
                    enemyRepeatDashUsed_[enemyIndex] = false;
                }
            }

            enemyVerticalVelocity = 0.0f;
            enemyGrounded_[enemyIndex] = false;
        }

        if (bossPhase_ == 2 && !enemyPositions_.empty() && enemyHealths_[0] > 0.0f) {
            if (!arePhase2AttackEffectsActive_ &&
                phase2TeleportState_ == Phase2TeleportState::Idle) {
                const float previousNextAttackTimer = phase2NextAttackTimer_;
                phase2NextAttackTimer_ -= deltaTime;
                if (previousNextAttackTimer >
                        kPhase2WeaponHideBeforeTelegraph &&
                    phase2NextAttackTimer_ <=
                        kPhase2WeaponHideBeforeTelegraph) {
                    dashSoundRequested_ = true;
                }
                if (phase2NextAttackTimer_ <= 0.0f) {
                    const int32_t minimumPatternIndex =
                        phase2TeleportPositionIndex_ == 0 ? 0 : 1;
                    std::uniform_int_distribution<int32_t> patternDistribution(
                        minimumPatternIndex,
                        3);
                    SpawnPhase2AttackPattern(static_cast<Phase2AttackPattern>(
                        patternDistribution(enemyRandomEngine_)));
                }
            }
            else if (arePhase2AttackEffectsActive_) {
                const bool isEightWay = phase2AttackPattern_ == Phase2AttackPattern::EightWay;
                const bool isFastSpin = phase2AttackPattern_ == Phase2AttackPattern::FastSpin;
                const bool isRadialAttack = isEightWay || isFastSpin;
                const bool isSideSweep =
                    phase2AttackPattern_ == Phase2AttackPattern::SideSweep;
                const float nonRadialTelegraphDuration = isSideSweep
                    ? kPhase2SideSweepTelegraphDuration
                    : kPhase2RainTelegraphDuration;
                const float attackTotalDuration = isEightWay
                    ? (isPhase2CloneActive_
                        ? kPhase2CloneEightWayTotalDuration
                        : kPhase2AttackTotalDuration)
                    : (isFastSpin
                        ? kPhase2FastSpinTotalDuration +
                            (isPhase2CloneActive_
                                ? kPhase2CloneTelegraphExtension
                                : 0.0f)
                        : nonRadialTelegraphDuration +
                            kPhase2RainAttackDuration +
                            (isPhase2CloneActive_
                                ? kPhase2CloneTelegraphExtension
                                : 0.0f));
                const float previousAttackTime = phase2AttackEffectElapsedTime_;
                phase2AttackEffectElapsedTime_ = (std::min)(
                    phase2AttackEffectElapsedTime_ + deltaTime,
                    attackTotalDuration);
                const float telegraphDuration =
                    GetPhase2AttackTelegraphDuration();
                const float blinkStartTime =
                    telegraphDuration - kPhase2TelegraphBlinkLeadTime;
                if (previousAttackTime < blinkStartTime &&
                    phase2AttackEffectElapsedTime_ >= blinkStartTime) {
                    alertSoundRequested_ = true;
                }
                if (!isRadialAttack &&
                    previousAttackTime < telegraphDuration &&
                    phase2AttackEffectElapsedTime_ >=
                        telegraphDuration) {
                    fallSoundRequested_ = true;
                }

                if (isRadialAttack) {
                    const Vector3& attackOrigin = isPhase2CloneActive_
                        ? kPhase2ClonePosition
                        : enemyPositions_[0];
                    const float rotationDelay = isFastSpin
                        ? telegraphDuration
                        : kPhase2AttackRotationDelay;
                    const float rotationSpeed = isFastSpin ? 2.2f : 0.35f;
                    const float rotationEaseInDuration = isFastSpin
                        ? kPhase2FastSpinRotationEaseInDuration
                        : kPhase2AttackRotationEaseInDuration;
                    if (phase2AttackEffectElapsedTime_ > rotationDelay) {
                        const float rotationStartTime = (std::max)(
                            previousAttackTime,
                            rotationDelay);
                        const float rotationDeltaTime =
                            phase2AttackEffectElapsedTime_ - rotationStartTime;
                        const float rotationTime =
                            phase2AttackEffectElapsedTime_ - rotationDelay;
                        const float rotationEase = std::clamp(
                            rotationTime / rotationEaseInDuration,
                            0.0f,
                            1.0f);
                        phase2AttackEffectRotation_ +=
                            rotationSpeed * phase2AttackEffectRotationDirection_ *
                            rotationEase * rotationDeltaTime;
                    }

                    const float spawnDistance = isFastSpin
                        ? kPhase2FastSpinSpawnDistance
                        : kPhase2AttackEffectSpawnDistance;

                    for (size_t effectIndex = 0; effectIndex < phase2AttackEffectActiveCount_; ++effectIndex) {
                        const float angle = phase2AttackEffectRotation_ +
                            2.0f * std::numbers::pi_v<float> *
                            static_cast<float>(effectIndex) /
                            static_cast<float>(phase2AttackEffectActiveCount_);
                        const Vector3 direction = { std::cos(angle), std::sin(angle), 0.0f };
                        phase2AttackEffectDirections_[effectIndex] = direction;
                        phase2AttackEffectPositions_[effectIndex] = {
                            attackOrigin.x + direction.x * spawnDistance,
                            attackOrigin.y + direction.y * spawnDistance,
                            attackOrigin.z,
                        };
                    }
                }
                else if (phase2AttackEffectElapsedTime_ >= telegraphDuration) {
                    const float attackProgress = std::clamp(
                        (phase2AttackEffectElapsedTime_ - telegraphDuration) /
                        kPhase2RainAttackDuration,
                        0.0f,
                        1.0f);
                    if (phase2AttackPattern_ == Phase2AttackPattern::SideSweep) {
                        for (size_t effectIndex = 0;
                            effectIndex < phase2AttackEffectActiveCount_;
                            ++effectIndex) {
                            const float direction =
                                phase2AttackEffectDirections_[effectIndex].x;
                            const float startX = direction > 0.0f
                                ? kPhase2SideSweepEndX
                                : kPhase2SideSweepStartX;
                            const float endX = direction > 0.0f
                                ? kPhase2SideSweepStartX
                                : kPhase2SideSweepEndX;
                            phase2AttackEffectPositions_[effectIndex].x =
                                startX + (endX - startX) * attackProgress;
                        }
                    }
                    else {
                        const float fallY = kPhase2RainStartY +
                            (kPhase2RainEndY - kPhase2RainStartY) * attackProgress;
                        for (size_t effectIndex = 0;
                            effectIndex < phase2AttackEffectActiveCount_;
                            ++effectIndex) {
                            phase2AttackEffectPositions_[effectIndex].y = fallY;
                        }
                    }
                }

                if (phase2AttackEffectElapsedTime_ >= attackTotalDuration) {
                    if (phase2AttackPattern_ ==
                            Phase2AttackPattern::SideSweep &&
                        phase2SideSweepWaveIndex_ == 0) {
                        // After the first four-lane set crosses the arena,
                        // launch one more four-lane set from the opposite side.
                        ++phase2SideSweepWaveIndex_;
                        phase2AttackEffectElapsedTime_ = 0.0f;
                        phase2AttackDamageCooldown_ = 0.0f;
                        phase2AttackEffectActiveCount_ = 4;
                        const float sweepDirection =
                            -phase2SideSweepFirstDirection_;
                        std::uniform_real_distribution<float> sweepYDistribution(
                            kPhase2SideSweepMinimumY,
                            kPhase2SideSweepMaximumY);
                        for (size_t effectIndex = 0;
                            effectIndex < phase2AttackEffectActiveCount_;
                            ++effectIndex) {
                            const bool targetsPlayer =
                                effectIndex ==
                                    phase2AttackEffectActiveCount_ - 1;
                            phase2AttackEffectDirections_[effectIndex] = {
                                sweepDirection,
                                0.0f,
                                0.0f,
                            };
                            phase2AttackEffectPositions_[effectIndex] = {
                                0.0f,
                                targetsPlayer
                                    ? std::clamp(
                                        playerPosition_.y,
                                        kPhase2SideSweepMinimumY,
                                        kPhase2SideSweepMaximumY)
                                    : sweepYDistribution(enemyRandomEngine_),
                                0.0f,
                            };
                        }
                    }
                    else {
                        const bool wasCloneAttack = isPhase2CloneActive_;
                        arePhase2AttackEffectsActive_ = false;
                        isPhase2CloneActive_ = false;
                        if (wasCloneAttack) {
                            // The clone vanishes after exactly one complete
                            // attack, then the real boss returns at the
                            // teleport destination.
                            isPhase2BossHiddenForClone_ = false;
                            phase2TeleportState_ =
                                Phase2TeleportState::Appearing;
                            phase2TeleportEffectTimer_ =
                                kPhase2TeleportEffectDuration;
                            teleportSoundRequested_ = true;
                        }
                        phase2NextAttackTimer_ =
                            kPhase2NextAttackDelay *
                                GetAttackIntervalScale();
                    }
                }
            }
        }

        // Damage the player when touching a living enemy. The cooldown prevents
        // continuous per-frame damage while their collision boxes overlap.
        constexpr float kContactDamage = 10.0f;
        constexpr float kEnemyAttackDamage = 10.0f;
        constexpr float kPhase2AttackEffectDamage = 10.0f;
        constexpr float kContactDamageInterval = 0.75f;
        constexpr float kPhase2AttackDamageInterval = 0.75f;
        constexpr float kEnemyHalfHeight = 0.812f;
        enemyContactDamageCooldown_ = (std::max)(0.0f, enemyContactDamageCooldown_ - deltaTime);
        phase2AttackDamageCooldown_ = (std::max)(0.0f, phase2AttackDamageCooldown_ - deltaTime);
        const float phase2TelegraphDuration =
            GetPhase2AttackTelegraphDuration();
        if (playerInvincibilityTimer_ <= 0.0f &&
            phase2AttackDamageCooldown_ <= 0.0f &&
            arePhase2AttackEffectsActive_ &&
            phase2AttackEffectElapsedTime_ >= phase2TelegraphDuration) {
            constexpr float kEffectCollisionRadius = 0.45f;
            constexpr float kPlayerCollisionRadius = 0.8f;
            constexpr float kCombinedCollisionRadius =
                kEffectCollisionRadius + kPlayerCollisionRadius;
            const float effectHalfLength = kPhase2AttackEffectModelHalfLength *
                (phase2AttackPattern_ == Phase2AttackPattern::EightWay
                    ? kPhase2AttackEffectScale
                    : (phase2AttackPattern_ == Phase2AttackPattern::FastSpin
                        ? kPhase2FastSpinLengthScale
                        : kPhase2RainLengthScale));
            for (size_t effectIndex = 0; effectIndex < phase2AttackEffectActiveCount_; ++effectIndex) {
                const Vector3& effectPosition = phase2AttackEffectPositions_[effectIndex];
                const Vector3& direction = phase2AttackEffectDirections_[effectIndex];
                const Vector3 segmentStart = {
                    effectPosition.x - direction.x * effectHalfLength,
                    effectPosition.y - direction.y * effectHalfLength,
                    effectPosition.z,
                };
                const Vector3 segmentEnd = {
                    effectPosition.x + direction.x * effectHalfLength,
                    effectPosition.y + direction.y * effectHalfLength,
                    effectPosition.z,
                };
                const Vector3 segment = {
                    segmentEnd.x - segmentStart.x,
                    segmentEnd.y - segmentStart.y,
                    0.0f,
                };
                const Vector3 startToPlayer = {
                    playerPosition_.x - segmentStart.x,
                    playerPosition_.y - segmentStart.y,
                    0.0f,
                };
                const float segmentLengthSquared =
                    segment.x * segment.x + segment.y * segment.y;
                const float closestPointRatio = segmentLengthSquared > 0.0f
                    ? std::clamp(
                        (startToPlayer.x * segment.x + startToPlayer.y * segment.y) /
                        segmentLengthSquared,
                        0.0f,
                        1.0f)
                    : 0.0f;
                const Vector3 closestPoint = {
                    segmentStart.x + segment.x * closestPointRatio,
                    segmentStart.y + segment.y * closestPointRatio,
                    segmentStart.z,
                };
                const float distanceX = playerPosition_.x - closestPoint.x;
                const float distanceY = playerPosition_.y - closestPoint.y;
                const float distanceZ = playerPosition_.z - closestPoint.z;
                const float distanceSquared =
                    distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ;
                const bool overlapsEffect = distanceSquared <=
                    kCombinedCollisionRadius * kCombinedCollisionRadius;
                if (overlapsEffect) {
                    playerHealth_ = (std::max)(
                        0.0f,
                        playerHealth_ - kPhase2AttackEffectDamage);
                    playerInvincibilityTimer_ = kPlayerInvincibilityDuration;
                    playerHitFlashTimer_ = kHitFlashDuration;
                    damageSoundRequested_ = true;
                    phase2AttackDamageCooldown_ = kPhase2AttackDamageInterval;
                    break;
                }
            }
        }
        if (!isTutorialMode_ &&
            playerInvincibilityTimer_ <= 0.0f &&
            enemyContactDamageCooldown_ <= 0.0f) {
            for (size_t enemyIndex = 0; enemyIndex < enemyPositions_.size(); ++enemyIndex) {
                if (enemyHealths_[enemyIndex] <= 0.0f) {
                    continue;
                }
                if (bossPhase_ == 2 &&
                    (phase2TeleportState_ != Phase2TeleportState::Idle ||
                        isPhase2BossHiddenForClone_)) {
                    continue;
                }

                const Vector3& enemyPosition = enemyPositions_[enemyIndex];
                const bool overlaps =
                    std::abs(playerPosition_.x - enemyPosition.x) <= kPlayerHalfWidth + kEnemyHalfWidth &&
                    std::abs(playerPosition_.y - enemyPosition.y) <= kPlayerHalfHeight + kEnemyHalfHeight &&
                    std::abs(playerPosition_.z - enemyPosition.z) <= kPlayerHalfWidth + kEnemyHalfWidth;
                if (overlaps) {
                    const bool isFirstAttackHit =
                        enemyBehaviors_[enemyIndex] == EnemyBehavior::Attack &&
                        !enemyAttackHitPlayers_[enemyIndex];
                    const float damage = isFirstAttackHit ? kEnemyAttackDamage : kContactDamage;
                    playerHealth_ = (std::max)(0.0f, playerHealth_ - damage);
                    playerInvincibilityTimer_ = kPlayerInvincibilityDuration;
                    playerHitFlashTimer_ = kHitFlashDuration;
                    damageSoundRequested_ = true;
                    if (isFirstAttackHit) {
                        enemyAttackHitPlayers_[enemyIndex] = true;
                    }
                    enemyContactDamageCooldown_ = kContactDamageInterval;
                    break;
                }
            }
        }

        constexpr float kProjectileSpeed = 28.0f;
        constexpr float kProjectileLifetime = 2.0f;
        constexpr float kProjectileSpawnDistance = 2.0f;
        constexpr float kProjectileSpawnHeightOffset = -0.15f;
        constexpr float kEnemyHalfWidthForProjectile = 0.8f;
        constexpr float kEnemyHalfHeightForProjectile = 0.812f;
        constexpr float kProjectileBlockCollisionRadius = 0.25f;
        constexpr float kChargeSecondsPerAmmo = 0.25f;

        const bool hasActiveChargedProjectile = std::any_of(
            projectiles_.begin(),
            projectiles_.end(),
            [](const Projectile& projectile) {
                return projectile.isActive && projectile.ammoCost > 1;
            });

        if (hasActiveChargedProjectile) {
            playerChargeTime_ = 0.0f;
            isPlayerCharging_ = false;
        }
        else if (attackHeld) {
            isPlayerCharging_ = true;
            playerChargeTime_ += deltaTime;
        }

        if (attackReleased && isPlayerCharging_) {
            const int32_t requestedAmmo = (std::min)(
                static_cast<int32_t>(playerChargeTime_ / kChargeSecondsPerAmmo) + 1,
                static_cast<int32_t>(kMaxProjectiles));
            const int32_t consumedAmmo = (std::min)(requestedAmmo, GetRemainingProjectileCount());
            const auto availableProjectile = std::find_if(
                projectiles_.begin(), projectiles_.end(),
                [](const Projectile& projectile) { return !projectile.isActive; });
            if (consumedAmmo > 0 && availableProjectile != projectiles_.end()) {
                const Vector3 direction = {
                    -std::sin(playerTargetRotationY_),
                    0.0f,
                    std::cos(playerTargetRotationY_),
                };
                availableProjectile->position = {
                    playerPosition_.x + direction.x * kProjectileSpawnDistance,
                    playerPosition_.y + kProjectileSpawnHeightOffset,
                    playerPosition_.z + direction.z * kProjectileSpawnDistance,
                };
                availableProjectile->direction = direction;
                availableProjectile->remainingTime = kProjectileLifetime;
                availableProjectile->damage = static_cast<float>(consumedAmmo);
                availableProjectile->scale = 0.25f + 0.05f * static_cast<float>(consumedAmmo - 1);
                availableProjectile->ammoCost = consumedAmmo;
                availableProjectile->isActive = true;
                firedProjectileAmmo_ = consumedAmmo;
            }
            playerChargeTime_ = 0.0f;
            isPlayerCharging_ = false;
        }

        for (Projectile& projectile : projectiles_) {
            if (!projectile.isActive) {
                continue;
            }

            projectile.position.x += projectile.direction.x * kProjectileSpeed * deltaTime;
            projectile.position.y += projectile.direction.y * kProjectileSpeed * deltaTime;
            projectile.position.z += projectile.direction.z * kProjectileSpeed * deltaTime;
            projectile.remainingTime -= deltaTime;

            const float projectileBoundaryRadius = (std::max)(
                projectile.scale,
                kProjectileBlockCollisionRadius);
            const bool touchesInvisibleBoundary =
                projectile.position.x - projectileBoundaryRadius <= -mapHalfWidth ||
                projectile.position.x + projectileBoundaryRadius >= mapHalfWidth ||
                projectile.position.y - projectileBoundaryRadius <= -mapHalfHeight ||
                projectile.position.y + projectileBoundaryRadius >= mapHalfHeight;
            bool shouldRemove =
                projectile.remainingTime <= 0.0f || touchesInvisibleBoundary;
            for (size_t enemyIndex = 0; !shouldRemove && enemyIndex < enemyPositions_.size(); ++enemyIndex) {
                if (enemyHealths_[enemyIndex] <= 0.0f) {
                    continue;
                }
                if (bossPhase_ == 2 &&
                    (phase2TeleportState_ != Phase2TeleportState::Idle ||
                        isPhase2BossHiddenForClone_)) {
                    continue;
                }

                const Vector3& enemyPosition = enemyPositions_[enemyIndex];
                const bool overlapsEnemy =
                    std::abs(projectile.position.x - enemyPosition.x) <= projectile.scale + kEnemyHalfWidthForProjectile &&
                    std::abs(projectile.position.y - enemyPosition.y) <= projectile.scale + kEnemyHalfHeightForProjectile &&
                    std::abs(projectile.position.z - enemyPosition.z) <= projectile.scale + kEnemyHalfWidthForProjectile;
                if (overlapsEnemy) {
                    if (!isTutorialMode_) {
                        const float previousEnemyHealth = enemyHealths_[enemyIndex];
                        enemyHealths_[enemyIndex] = (std::max)(
                            0.0f,
                            previousEnemyHealth - projectile.damage);
                        if (bossPhase_ == 2 &&
                            previousEnemyHealth > 0.0f &&
                            enemyHealths_[enemyIndex] <= 0.0f) {
                            enemyDamageSoundRequested_ = true;
                        }
                    }
                    else if (tutorialStep_ == TutorialStep::Attack) {
                        tutorialStep_ = TutorialStep::ChargeAttack;
                    }
                    else if (tutorialStep_ == TutorialStep::ChargeAttack &&
                        projectile.ammoCost >= kTutorialFullChargeAmmo) {
                        tutorialStep_ = TutorialStep::Complete;
                    }
                    enemyHitFlashTimer_ = kHitFlashDuration;
                    shouldRemove = true;
                }
            }

            for (const TileMapCell& cell : cells_) {
                if (shouldRemove) {
                    break;
                }

                const bool overlapsWall =
                    std::abs(projectile.position.x - cell.worldPosition.x) <= kProjectileBlockCollisionRadius + kTileHalfExtent &&
                    std::abs(projectile.position.y - cell.worldPosition.y) <= kProjectileBlockCollisionRadius + kTileHalfExtent &&
                    std::abs(projectile.position.z - cell.worldPosition.z) <= kProjectileBlockCollisionRadius + kTileHalfExtent;
                shouldRemove = overlapsWall;
            }

            if (shouldRemove) {
                projectile.isActive = false;
                projectile.ammoCost = 0;
            }
        }

        if (isTutorialMode_) {
            tutorialIconAnimationTime_ += deltaTime;
            if (tutorialStep_ == TutorialStep::Move &&
                std::abs(playerPosition_.x - tutorialStartPlayerX_) >=
                    kTutorialMoveDistance) {
                tutorialStep_ = TutorialStep::Jump;
                hasTutorialJumpStarted_ = false;
            }
            else if (tutorialStep_ == TutorialStep::Jump) {
                if (!isPlayerGrounded_) {
                    hasTutorialJumpStarted_ = true;
                }
                else if (hasTutorialJumpStarted_) {
                    tutorialStep_ = TutorialStep::Attack;
                }
            }

            if (tutorialStep_ == TutorialStep::Attack) {
                tutorialAttackHintTimer_ += deltaTime;
            }
            else {
                tutorialAttackHintTimer_ = 0.0f;
            }
        }

    }

    void GameScene::DrawSkyDome(
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
        const Matrix4x4& viewMatrix,
        const Matrix4x4& projectionMatrix) {
        commandList->SetGraphicsRootConstantBufferView(
            2,
            lightConstantBuffer_.GetGPUVirtualAddress());
        // Keep the dome centered on the camera so its edge can never enter the view.
        Matrix4x4 skyDomeViewMatrix = viewMatrix;
        skyDomeViewMatrix.m[3][0] = 0.0f;
        skyDomeViewMatrix.m[3][1] = 0.0f;
        skyDomeViewMatrix.m[3][2] = 0.0f;
        skyDomeViewMatrix.m[3][3] = 1.0f;
        const Matrix4x4 skyDomeWorldMatrix = MakeAffineMatrix(
            { 1.0f, 1.0f, 1.0f },
            { 0.0f, skyDomeRotationY_, 0.0f },
            { 0.0f, 0.0f, 0.0f });
        skyDomeTransformConstantBuffer_->World = skyDomeWorldMatrix;
        skyDomeTransformConstantBuffer_->WVP = Multiply(
            Multiply(skyDomeWorldMatrix, skyDomeViewMatrix), projectionMatrix);
        commandList->SetGraphicsRootConstantBufferView(
            0,
            skyDomeMaterialConstantBuffer_.GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(
            1,
            skyDomeTransformConstantBuffer_.GetGPUVirtualAddress());
        if (kDrawSkyDome) {
            skyDomeModel_.Draw(commandList, &textureManager_);
        }
    }

    void GameScene::Draw(
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
        const Matrix4x4& viewMatrix,
        const Matrix4x4& projectionMatrix,
        float elapsedGameTimeSeconds,
        bool showGameTimer)
    {
        DrawSkyDome(commandList, viewMatrix, projectionMatrix);

        constexpr Vector3 kCubeScale = { 1.0f, 1.0f, 1.0f };
        for (size_t cellIndex = 0; cellIndex < cells_.size(); ++cellIndex) {
            const TileMapCell& cell = cells_[cellIndex];
            TransformationMatrixConstantBuffer& transformConstantBuffer = *transformConstantBuffers_[cellIndex];
            const Matrix4x4 worldMatrix = MakeAffineMatrix(
                kCubeScale,
                { 0.0f, 0.0f, 0.0f },
                cell.worldPosition);
            transformConstantBuffer->World = worldMatrix;
            transformConstantBuffer->WVP = Multiply(Multiply(worldMatrix, viewMatrix), projectionMatrix);
            commandList->SetGraphicsRootConstantBufferView(
                0,
                cellIndex == static_cast<size_t>(selectedCellIndex_)
                    ? selectionMaterialConstantBuffer_.GetGPUVirtualAddress()
                    : materialConstantBuffer_.GetGPUVirtualAddress());
            commandList->SetGraphicsRootConstantBufferView(1, transformConstantBuffer.GetGPUVirtualAddress());
            cubeModel_.Draw(commandList, &textureManager_);
        }

        constexpr Vector3 kPlayerScale = { 2.0f, 2.0f, 2.0f };
        const Vector3 playerRotation = {
            playerDeathRotationX_,
            playerRotationY_,
            0.0f,
        };
        const Matrix4x4 playerWorldMatrix = MakeAffineMatrix(
            kPlayerScale,
            playerRotation,
            playerPosition_);
        playerTransformConstantBuffer_->World = playerWorldMatrix;
        playerTransformConstantBuffer_->WVP = Multiply(
            Multiply(playerWorldMatrix, viewMatrix), projectionMatrix);
        playerMaterialConstantBuffer_->color = playerHitFlashTimer_ > 0.0f
            ? Vector4{ 1.0f, 0.1f, 0.1f, 1.0f }
            : Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
        const bool isPlayerBlinkHidden =
            playerHitFlashTimer_ <= 0.0f &&
            playerInvincibilityTimer_ > 0.0f &&
            std::fmod(playerInvincibilityTimer_, kPlayerBlinkInterval * 2.0f) <
                kPlayerBlinkInterval;
        if (!isPlayerBlinkHidden) {
            commandList->SetGraphicsRootConstantBufferView(
                0,
                playerMaterialConstantBuffer_.GetGPUVirtualAddress());
            commandList->SetGraphicsRootConstantBufferView(
                1,
                playerTransformConstantBuffer_.GetGPUVirtualAddress());
            playerModel_.Draw(commandList, &textureManager_);
        }

        if (isPlayerCharging_ && GetRemainingProjectileCount() > 0) {
            constexpr float kChargeSecondsPerAmmo = 0.25f;
            constexpr float kBaseProjectileScale = 0.25f;
            constexpr float kScalePerAmmo = 0.05f;
            constexpr float kProjectileSpawnDistance = 2.0f;
            constexpr float kProjectileSpawnHeightOffset = -0.15f;
            const float maximumChargeSteps = static_cast<float>(GetRemainingProjectileCount() - 1);
            const float chargeSteps = (std::min)(
                playerChargeTime_ / kChargeSecondsPerAmmo,
                maximumChargeSteps);
            const float projectileScale = kBaseProjectileScale + kScalePerAmmo * chargeSteps;
            const Vector3 direction = {
                -std::sin(playerTargetRotationY_),
                0.0f,
                std::cos(playerTargetRotationY_),
            };
            const Vector3 projectilePosition = {
                playerPosition_.x + direction.x * kProjectileSpawnDistance,
                playerPosition_.y + kProjectileSpawnHeightOffset,
                playerPosition_.z + direction.z * kProjectileSpawnDistance,
            };
            const Matrix4x4 projectileWorldMatrix = MakeAffineMatrix(
                { projectileScale, projectileScale, projectileScale },
                { 0.0f, 0.0f, 0.0f },
                projectilePosition);
            chargingProjectileTransformConstantBuffer_->World = projectileWorldMatrix;
            chargingProjectileTransformConstantBuffer_->WVP = Multiply(
                Multiply(projectileWorldMatrix, viewMatrix), projectionMatrix);
            commandList->SetGraphicsRootConstantBufferView(
                0,
                projectileMaterialConstantBuffer_.GetGPUVirtualAddress());
            commandList->SetGraphicsRootConstantBufferView(
                1,
                chargingProjectileTransformConstantBuffer_.GetGPUVirtualAddress());
            bulletModel_.Draw(commandList, &textureManager_);
        }

        if (bossPhase_ == 1) {
            constexpr float kDashTrailWidthScale = 1.0f;
            constexpr float kDashTrailSideOffset = 0.4f;
            constexpr std::array<float, 3> kDashTrailOffsets = {
                0.0f,
                -kDashTrailSideOffset,
                kDashTrailSideOffset,
            };
            constexpr float kDashTrailRenderZ = kPhase2EffectRenderZ;
            enemyAttackEffectMaterialConstantBuffer_->color = {
                1.0f, 1.0f, 1.0f, 1.0f,
            };
            for (size_t enemyIndex = 0; enemyIndex < enemyPositions_.size(); ++enemyIndex) {
                if (enemyBehaviors_[enemyIndex] != EnemyBehavior::Attack) {
                    continue;
                }
                const size_t firstTrailBufferIndex =
                    enemyIndex * kDashTrailOffsets.size();
                if (firstTrailBufferIndex + kDashTrailOffsets.size() >
                    enemyAttackEffectTransformConstantBuffers_.size()) {
                    continue;
                }

                const Vector3& startPosition = enemyAttackStartPositions_[enemyIndex];
                const Vector3& currentPosition = enemyPositions_[enemyIndex];
                const Vector3 trailVector = {
                    currentPosition.x - startPosition.x,
                    currentPosition.y - startPosition.y,
                    0.0f,
                };
                const float trailLength = std::sqrt(
                    trailVector.x * trailVector.x + trailVector.y * trailVector.y);
                if (trailLength <= 0.01f) {
                    continue;
                }

                const Vector3 trailDirection = {
                    trailVector.x / trailLength,
                    trailVector.y / trailLength,
                    0.0f,
                };
                const float trailLengthScale =
                    trailLength / (2.0f * kPhase2AttackEffectModelHalfLength);
                const Vector3 trailPerpendicular = {
                    -trailDirection.y,
                    trailDirection.x,
                    0.0f,
                };
                const Vector3 trailCenter = {
                    (startPosition.x + currentPosition.x) * 0.5f,
                    (startPosition.y + currentPosition.y) * 0.5f,
                    kDashTrailRenderZ,
                };

                for (size_t trailIndex = 0;
                    trailIndex < kDashTrailOffsets.size();
                    ++trailIndex) {
                    const float trailOffset = kDashTrailOffsets[trailIndex];
                    Matrix4x4 trailWorldMatrix = MakeIdentityMatrix();
                    trailWorldMatrix.m[0][0] =
                        trailDirection.y * kDashTrailWidthScale;
                    trailWorldMatrix.m[0][1] =
                        -trailDirection.x * kDashTrailWidthScale;
                    trailWorldMatrix.m[1][0] = trailDirection.x * trailLengthScale;
                    trailWorldMatrix.m[1][1] = trailDirection.y * trailLengthScale;
                    trailWorldMatrix.m[2][2] = kDashTrailWidthScale;
                    trailWorldMatrix.m[3][0] =
                        trailCenter.x + trailPerpendicular.x * trailOffset;
                    trailWorldMatrix.m[3][1] =
                        trailCenter.y + trailPerpendicular.y * trailOffset;
                    trailWorldMatrix.m[3][2] = trailCenter.z;

                    TransformationMatrixConstantBuffer& trailTransformConstantBuffer =
                        enemyAttackEffectTransformConstantBuffers_[
                            firstTrailBufferIndex + trailIndex];
                    trailTransformConstantBuffer->World = trailWorldMatrix;
                    trailTransformConstantBuffer->WVP = Multiply(
                        Multiply(trailWorldMatrix, viewMatrix), projectionMatrix);
                    commandList->SetGraphicsRootConstantBufferView(
                        0,
                        enemyAttackEffectMaterialConstantBuffer_.GetGPUVirtualAddress());
                    commandList->SetGraphicsRootConstantBufferView(
                        1,
                        trailTransformConstantBuffer.GetGPUVirtualAddress());
                    enemyAttackEffectModel_.Draw(commandList, &textureManager_);
                }
            }
        }

        constexpr Vector3 kEnemyBaseScale = { 2.0f, 2.0f, 2.0f };
        float phase2TeleportVisualScale = 1.0f;
        if (phase2TeleportState_ == Phase2TeleportState::Disappearing) {
            phase2TeleportVisualScale =
                phase2TeleportEffectTimer_ / kPhase2TeleportEffectDuration;
        }
        else if (phase2TeleportState_ == Phase2TeleportState::Appearing) {
            phase2TeleportVisualScale =
                1.0f - phase2TeleportEffectTimer_ / kPhase2TeleportEffectDuration;
        }
        phase2TeleportVisualScale = std::clamp(
            phase2TeleportVisualScale,
            0.02f,
            1.0f);
        const bool shouldDrawPhase2Weapon =
            bossPhase_ == 2 &&
            !isPhase2IntroActive_ &&
            !isEnemyDefeatAnimationActive_ &&
            !arePhase2AttackEffectsActive_ &&
            phase2TeleportState_ == Phase2TeleportState::Idle &&
            phase2NextAttackTimer_ > kPhase2WeaponHideBeforeTelegraph;
        constexpr float kTutorialAttackHintDelay = 3.0f;
        constexpr float kTutorialAttackHintBlinkInterval = 0.15f;
        const bool shouldShowTutorialAttackHint =
            isTutorialMode_ &&
            tutorialStep_ == TutorialStep::Attack &&
            tutorialAttackHintTimer_ >= kTutorialAttackHintDelay &&
            std::fmod(
                tutorialAttackHintTimer_ - kTutorialAttackHintDelay,
                kTutorialAttackHintBlinkInterval * 2.0f) <
                kTutorialAttackHintBlinkInterval;
        const float phase1TransformProgress = std::clamp(
            (enemyDefeatAnimationTimer_ -
                kPhase1TransformEnergyBuildStartTime) /
                (kPhase1TransformExpansionEndTime -
                    kPhase1TransformEnergyBuildStartTime),
            0.0f,
            1.0f);
        const float enemyDefeatFlashInterval = bossPhase_ == 1
            ? 0.18f - phase1TransformProgress * 0.10f
            : 0.12f;
        const bool shouldShowEnemyDefeatFlash =
            isEnemyDefeatAnimationActive_ &&
            std::fmod(enemyDefeatAnimationTimer_, enemyDefeatFlashInterval) <
                enemyDefeatFlashInterval * 0.5f;
        const bool shouldShowPhase2IntroFlash =
            isPhase2IntroActive_ &&
            phase2IntroElapsedTime_ < kPhase2IntroSettleEndTime &&
            std::fmod(phase2IntroElapsedTime_, 0.10f) < 0.05f;
        const bool shouldHoldPhaseTransitionFlash =
            bossPhase_ == 1 && isEnemyDefeatAnimationFinished_;
        enemyMaterialConstantBuffer_->useTexture =
            shouldShowTutorialAttackHint || shouldShowEnemyDefeatFlash ||
                shouldShowPhase2IntroFlash || shouldHoldPhaseTransitionFlash
            ? 0
            : 1;
        enemyMaterialConstantBuffer_->color =
            shouldShowPhase2IntroFlash || shouldHoldPhaseTransitionFlash
            ? Vector4{ 0.55f, 0.95f, 1.0f, phase2TeleportVisualScale }
            : (shouldShowEnemyDefeatFlash
            ? (bossPhase_ == 1
                ? (enemyDefeatAnimationTimer_ >=
                    kPhase1TransformModelSwapTime
                    ? Vector4{ 1.0f, 1.0f, 1.0f, phase2TeleportVisualScale }
                    : Vector4{ 0.35f, 0.85f, 1.0f, phase2TeleportVisualScale })
                : Vector4{ 1.0f, 1.0f, 1.0f, phase2TeleportVisualScale })
            : (enemyHitFlashTimer_ > 0.0f
                ? Vector4{ 1.0f, 0.1f, 0.1f, phase2TeleportVisualScale }
                : Vector4{ 1.0f, 1.0f, 1.0f, phase2TeleportVisualScale }));
        for (size_t enemyIndex = 0; enemyIndex < enemyPositions_.size(); ++enemyIndex) {
            if (isEnemyDefeatAnimationFinished_ && bossPhase_ == 2) {
                continue;
            }
            if (bossPhase_ == 2 && isPhase2BossHiddenForClone_) {
                continue;
            }
            const Vector3& enemyPosition = enemyPositions_[enemyIndex];
            float defeatVisualScale = 1.0f;
            Vector3 defeatVisualAxisScale{ 1.0f, 1.0f, 1.0f };
            Vector3 defeatVisualOffset{};
            float defeatRotationZ = 0.0f;
            if (isPhase2IntroActive_) {
                const float settleProgress = std::clamp(
                    phase2IntroElapsedTime_ / kPhase2IntroSettleEndTime,
                    0.0f,
                    1.0f);
                const float easedSettle = settleProgress * settleProgress *
                    (3.0f - 2.0f * settleProgress);
                defeatVisualScale = kPhaseTransitionExpandedScale -
                    (kPhaseTransitionExpandedScale - 1.0f) * easedSettle;
                defeatVisualOffset.x =
                    std::sin(phase2IntroElapsedTime_ * 22.0f) *
                    0.10f * (1.0f - easedSettle);
            }
            else if (isEnemyDefeatAnimationActive_ ||
                (bossPhase_ == 1 && isEnemyDefeatAnimationFinished_)) {
                if (bossPhase_ == 1) {
                    const float returnProgress = std::clamp(
                        (enemyDefeatAnimationTimer_ -
                            kPhase1TransformReturnStartTime) /
                            (kPhase1TransformReturnEndTime -
                                kPhase1TransformReturnStartTime),
                        0.0f,
                        1.0f);
                    const float easedReturn = returnProgress * returnProgress *
                        (3.0f - 2.0f * returnProgress);
                    const float transformationShakeFade = 1.0f - std::clamp(
                        (enemyDefeatAnimationTimer_ -
                            kPhase1TransformModelSwapTime) /
                            (kPhase1TransformExpansionEndTime -
                                kPhase1TransformModelSwapTime),
                        0.0f,
                        1.0f);
                    defeatVisualOffset = {
                        -enemyPosition.x * easedReturn +
                        std::sin(enemyDefeatAnimationTimer_ * 34.0f) *
                            (0.02f + phase1TransformProgress * 0.16f) *
                                transformationShakeFade,
                        -enemyPosition.y * easedReturn,
                        0.0f,
                    };
                    if (enemyDefeatAnimationTimer_ <
                        kPhase1TransformCollapseStartTime) {
                        const float impactProgress = std::clamp(
                            enemyDefeatAnimationTimer_ /
                                kPhase1TransformEnergyBuildStartTime,
                            0.0f,
                            1.0f);
                        defeatVisualScale = 1.0f +
                            std::sin(impactProgress *
                                std::numbers::pi_v<float>) * 0.10f +
                            std::sin(enemyDefeatAnimationTimer_ * 15.0f) *
                                0.025f * phase1TransformProgress;
                        defeatRotationZ = phase1TransformProgress * 0.18f;
                    }
                    else if (enemyDefeatAnimationTimer_ <
                        kPhase1TransformModelSwapTime) {
                        const float collapseProgress = std::clamp(
                            (enemyDefeatAnimationTimer_ -
                                kPhase1TransformCollapseStartTime) /
                                (kPhase1TransformModelSwapTime -
                                    kPhase1TransformCollapseStartTime),
                            0.0f,
                            1.0f);
                        const float easedCollapse =
                            collapseProgress * collapseProgress *
                            (3.0f - 2.0f * collapseProgress);
                        defeatVisualScale = 1.0f -
                            (1.0f - kPhaseTransitionCoreScale) *
                                easedCollapse;
                        defeatVisualAxisScale = {
                            1.0f - 0.55f * easedCollapse,
                            1.0f + 0.65f * easedCollapse,
                            1.0f,
                        };
                        defeatRotationZ = kPhaseTransitionRotation *
                            easedCollapse;
                    }
                    else {
                        const float expansionProgress = std::clamp(
                            (enemyDefeatAnimationTimer_ -
                                kPhase1TransformModelSwapTime) /
                                (kPhase1TransformExpansionEndTime -
                                    kPhase1TransformModelSwapTime),
                            0.0f,
                            1.0f);
                        const float easedExpansion = 1.0f - std::pow(
                            1.0f - expansionProgress,
                            3.0f);
                        defeatVisualScale = kPhaseTransitionCoreScale +
                            (kPhaseTransitionExpandedScale -
                                kPhaseTransitionCoreScale) * easedExpansion;
                        defeatVisualAxisScale = {
                            0.45f + 0.55f * easedExpansion,
                            1.65f - 0.65f * easedExpansion,
                            1.0f,
                        };
                        defeatRotationZ = kPhaseTransitionRotation *
                            (1.0f - easedExpansion);
                    }
                }
                else {
                    if (enemyDefeatAnimationTimer_ >=
                        kPhase2DefeatShrinkStartTime) {
                        const float shrinkProgress = std::clamp(
                            (enemyDefeatAnimationTimer_ -
                                kPhase2DefeatShrinkStartTime) /
                                (kPhase2DefeatDisappearTime -
                                    kPhase2DefeatShrinkStartTime),
                            0.0f,
                            1.0f);
                        const float easedShrink =
                            shrinkProgress * shrinkProgress *
                            (3.0f - 2.0f * shrinkProgress);
                        defeatVisualScale = 1.0f - easedShrink;
                    }
                    else {
                        defeatVisualScale = 1.0f +
                            std::sin(enemyDefeatAnimationTimer_ * 18.0f) *
                                0.06f;
                    }
                    const float shakeStrength = 0.08f +
                        0.28f * std::clamp(
                            enemyDefeatAnimationTimer_ /
                                kPhase2DefeatShrinkStartTime,
                            0.0f,
                            1.0f);
                    defeatVisualOffset = {
                        std::sin(enemyDefeatAnimationTimer_ * 43.0f) *
                            shakeStrength,
                        std::cos(enemyDefeatAnimationTimer_ * 37.0f) *
                            shakeStrength,
                        0.0f,
                    };
                    defeatRotationZ = enemyDefeatAnimationTimer_ *
                        enemyDefeatAnimationTimer_ * 2.2f;
                }
            }
            const bool isPhase1TransformationShowingPhase2 =
                bossPhase_ == 1 &&
                (isEnemyDefeatAnimationActive_ ||
                    isEnemyDefeatAnimationFinished_) &&
                enemyDefeatAnimationTimer_ >=
                    kPhase1TransformModelSwapTime;
            const bool isPhase2Visual =
                bossPhase_ == 2 || isPhase1TransformationShowingPhase2;
            Vector3 enemyScale = bossPhase_ == 2
                ? Vector3{
                    kEnemyBaseScale.x * phase2TeleportVisualScale,
                    kEnemyBaseScale.y * phase2TeleportVisualScale,
                    kEnemyBaseScale.z * phase2TeleportVisualScale,
                }
                : kEnemyBaseScale;
            enemyScale.x *= defeatVisualScale * defeatVisualAxisScale.x;
            enemyScale.y *= defeatVisualScale * defeatVisualAxisScale.y;
            enemyScale.z *= defeatVisualScale * defeatVisualAxisScale.z;
            const float phase2BodyBob =
                isPhase2Visual && !isPhase2IntroActive_ &&
                    !isEnemyDefeatAnimationActive_
                ? std::sin(elapsedGameTimeSeconds * 1.5f) * 0.20f
                : 0.0f;
            Vector3 displayedEnemyPosition = isPhase2Visual
                ? Vector3{
                    enemyPosition.x,
                    enemyPosition.y + phase2BodyBob,
                    enemyPosition.z,
                }
                : enemyPosition;
            displayedEnemyPosition.x += defeatVisualOffset.x;
            displayedEnemyPosition.y += defeatVisualOffset.y;
            const Matrix4x4 enemyWorldMatrix = MakeAffineMatrix(
                enemyScale,
                {
                    0.0f,
                    std::numbers::pi_v<float>,
                    defeatRotationZ,
                },
                displayedEnemyPosition);
            TransformationMatrixConstantBuffer& enemyTransformConstantBuffer =
                *enemyTransformConstantBuffers_[enemyIndex];
            enemyTransformConstantBuffer->World = enemyWorldMatrix;
            enemyTransformConstantBuffer->WVP = Multiply(
                Multiply(enemyWorldMatrix, viewMatrix), projectionMatrix);
            commandList->SetGraphicsRootConstantBufferView(
                0,
                enemyMaterialConstantBuffer_.GetGPUVirtualAddress());
            commandList->SetGraphicsRootConstantBufferView(
                1,
                enemyTransformConstantBuffer.GetGPUVirtualAddress());
            if (isPhase2Visual) {
                enemyPhase2BodyModel_.Draw(commandList, &textureManager_);
                enemyPhase2Body2Model_.Draw(commandList, &textureManager_);

                if (shouldDrawPhase2Weapon) {
                    const float weaponBob =
                        std::sin(elapsedGameTimeSeconds * 2.0f) * 0.35f;
                    const float weaponRotation = elapsedGameTimeSeconds * 1.5f;
                    const Matrix4x4 weaponWorldMatrix = MakeAffineMatrix(
                        enemyScale,
                        { 0.0f, std::numbers::pi_v<float> + weaponRotation, 0.0f },
                        {
                            enemyPosition.x,
                            enemyPosition.y + weaponBob,
                            enemyPosition.z,
                        });
                    TransformationMatrixConstantBuffer& weaponTransformConstantBuffer =
                        *enemyPhase2WeaponTransformConstantBuffers_[enemyIndex];
                    weaponTransformConstantBuffer->World = weaponWorldMatrix;
                    weaponTransformConstantBuffer->WVP = Multiply(
                        Multiply(weaponWorldMatrix, viewMatrix), projectionMatrix);
                    commandList->SetGraphicsRootConstantBufferView(
                        1,
                        weaponTransformConstantBuffer.GetGPUVirtualAddress());
                    enemyPhase2WeaponModel_.Draw(commandList, &textureManager_);
                }
            }
            else {
                enemyModel_.Draw(commandList, &textureManager_);
            }
        }

        if (bossPhase_ == 2 &&
            isPhase2CloneActive_ &&
            !isEnemyDefeatAnimationActive_ &&
            !enemyHealths_.empty() &&
            enemyHealths_[0] > 0.0f) {
            const float cloneBob =
                std::sin(elapsedGameTimeSeconds * 1.8f) * 0.15f;
            const Matrix4x4 cloneWorldMatrix = MakeAffineMatrix(
                kEnemyBaseScale,
                { 0.0f, std::numbers::pi_v<float>, 0.0f },
                {
                    kPhase2ClonePosition.x,
                    kPhase2ClonePosition.y + cloneBob,
                    kPhase2ClonePosition.z,
                });
            phase2CloneTransformConstantBuffer_->World = cloneWorldMatrix;
            phase2CloneTransformConstantBuffer_->WVP = Multiply(
                Multiply(cloneWorldMatrix, viewMatrix),
                projectionMatrix);
            commandList->SetGraphicsRootConstantBufferView(
                0,
                phase2CloneMaterialConstantBuffer_.GetGPUVirtualAddress());
            commandList->SetGraphicsRootConstantBufferView(
                1,
                phase2CloneTransformConstantBuffer_.GetGPUVirtualAddress());
            enemyPhase2BodyModel_.Draw(commandList, &textureManager_);
            enemyPhase2Body2Model_.Draw(commandList, &textureManager_);
        }

        if (bossPhase_ == 2 &&
            phase2TeleportState_ != Phase2TeleportState::Idle &&
            !enemyPositions_.empty()) {
            constexpr size_t kTeleportRayCount = 8;
            constexpr float kTeleportRayLengthScale = 0.025f;
            constexpr float kTeleportRayWidthScale = 0.8f;
            const float teleportProgress = std::clamp(
                1.0f - phase2TeleportEffectTimer_ /
                    kPhase2TeleportEffectDuration,
                0.0f,
                1.0f);
            const float teleportRayRadius =
                phase2TeleportState_ == Phase2TeleportState::Disappearing
                ? 1.0f + 2.0f * teleportProgress
                : 3.0f - 2.0f * teleportProgress;
            enemyAttackEffectMaterialConstantBuffer_->color = {
                1.0f, 1.0f, 1.0f, 1.0f,
            };
            for (size_t rayIndex = 0; rayIndex < kTeleportRayCount; ++rayIndex) {
                const float angle =
                    2.0f * std::numbers::pi_v<float> *
                    static_cast<float>(rayIndex) /
                    static_cast<float>(kTeleportRayCount) +
                    teleportProgress * 0.35f;
                const Vector3 rayDirection = {
                    std::cos(angle),
                    std::sin(angle),
                    0.0f,
                };
                Matrix4x4 rayWorldMatrix = MakeIdentityMatrix();
                rayWorldMatrix.m[0][0] =
                    rayDirection.y * kTeleportRayWidthScale;
                rayWorldMatrix.m[0][1] =
                    -rayDirection.x * kTeleportRayWidthScale;
                rayWorldMatrix.m[1][0] =
                    rayDirection.x * kTeleportRayLengthScale;
                rayWorldMatrix.m[1][1] =
                    rayDirection.y * kTeleportRayLengthScale;
                rayWorldMatrix.m[2][2] = kTeleportRayWidthScale;
                rayWorldMatrix.m[3][0] = enemyPositions_[0].x +
                    rayDirection.x * teleportRayRadius;
                rayWorldMatrix.m[3][1] = enemyPositions_[0].y +
                    rayDirection.y * teleportRayRadius;
                rayWorldMatrix.m[3][2] = kPhase2EffectRenderZ;

                TransformationMatrixConstantBuffer& rayTransformConstantBuffer =
                    enemyAttackEffectTransformConstantBuffers_[rayIndex];
                rayTransformConstantBuffer->World = rayWorldMatrix;
                rayTransformConstantBuffer->WVP = Multiply(
                    Multiply(rayWorldMatrix, viewMatrix), projectionMatrix);
                commandList->SetGraphicsRootConstantBufferView(
                    0,
                    enemyAttackEffectMaterialConstantBuffer_.GetGPUVirtualAddress());
                commandList->SetGraphicsRootConstantBufferView(
                    1,
                    rayTransformConstantBuffer.GetGPUVirtualAddress());
                enemyAttackEffectModel_.Draw(commandList, &textureManager_);
            }
        }

        if (arePhase2AttackEffectsActive_) {
            MaterialConstantBuffer& attackEffectMaterialConstantBuffer =
                isPhase2CloneActive_
                ? phase2CloneAttackEffectMaterialConstantBuffer_
                : enemyAttackEffectMaterialConstantBuffer_;
            auto& attackEffectTransformConstantBuffers =
                isPhase2CloneActive_
                ? phase2CloneAttackEffectTransformConstantBuffers_
                : enemyAttackEffectTransformConstantBuffers_;
            const Vector3& attackOrigin = isPhase2CloneActive_
                ? kPhase2ClonePosition
                : enemyPositions_[0];
            const bool isFastSpin =
                phase2AttackPattern_ == Phase2AttackPattern::FastSpin;
            const float telegraphDuration =
                GetPhase2AttackTelegraphDuration();
            const bool isTelegraph =
                phase2AttackEffectElapsedTime_ < telegraphDuration;
            const float telegraphRemainingTime =
                telegraphDuration - phase2AttackEffectElapsedTime_;
            const bool isTelegraphBlinking =
                isTelegraph &&
                telegraphRemainingTime <= kPhase2TelegraphBlinkLeadTime;
            const bool isTelegraphVisible =
                !isTelegraphBlinking ||
                std::fmod(
                    phase2AttackEffectElapsedTime_,
                    kPhase2TelegraphBlinkInterval * 2.0f) <
                    kPhase2TelegraphBlinkInterval;
            if (isTelegraph) {
                attackEffectMaterialConstantBuffer->color = {
                    1.0f,
                    1.0f,
                    0.0f,
                    1.0f,
                };
            }
            else {
                attackEffectMaterialConstantBuffer->color = {
                    1.0f,
                    0.15f,
                    0.05f,
                    1.0f,
                };
            }
            if (isTelegraphVisible &&
                isTelegraph &&
                isFastSpin &&
                !enemyPositions_.empty()) {
                const float circleSegmentLength =
                    2.0f * kPhase2FastSpinAttackRadius *
                    std::sin(std::numbers::pi_v<float> /
                        static_cast<float>(kPhase2CircleTelegraphSegmentCount));
                const float circleSegmentLengthScale = circleSegmentLength /
                    (2.0f * kPhase2AttackEffectModelHalfLength);
                for (size_t segmentIndex = 0;
                    segmentIndex < kPhase2CircleTelegraphSegmentCount;
                    ++segmentIndex) {
                    const float angle =
                        2.0f * std::numbers::pi_v<float> *
                        (static_cast<float>(segmentIndex) + 0.5f) /
                        static_cast<float>(kPhase2CircleTelegraphSegmentCount);
                    const Vector3 radialDirection = {
                        std::cos(angle),
                        std::sin(angle),
                        0.0f,
                    };
                    const Vector3 tangentDirection = {
                        -radialDirection.y,
                        radialDirection.x,
                        0.0f,
                    };
                    Matrix4x4 circleSegmentWorldMatrix = MakeIdentityMatrix();
                    circleSegmentWorldMatrix.m[0][0] =
                        tangentDirection.y * kPhase2TelegraphWidthScale;
                    circleSegmentWorldMatrix.m[0][1] =
                        -tangentDirection.x * kPhase2TelegraphWidthScale;
                    circleSegmentWorldMatrix.m[1][0] =
                        tangentDirection.x * circleSegmentLengthScale;
                    circleSegmentWorldMatrix.m[1][1] =
                        tangentDirection.y * circleSegmentLengthScale;
                    circleSegmentWorldMatrix.m[2][2] = kPhase2TelegraphWidthScale;
                    circleSegmentWorldMatrix.m[3][0] = attackOrigin.x +
                        radialDirection.x * kPhase2FastSpinAttackRadius;
                    circleSegmentWorldMatrix.m[3][1] = attackOrigin.y +
                        radialDirection.y * kPhase2FastSpinAttackRadius;
                    circleSegmentWorldMatrix.m[3][2] = kPhase2EffectRenderZ;
                    TransformationMatrixConstantBuffer& circleTransformConstantBuffer =
                        attackEffectTransformConstantBuffers[segmentIndex];
                    circleTransformConstantBuffer->World = circleSegmentWorldMatrix;
                    circleTransformConstantBuffer->WVP = Multiply(
                        Multiply(circleSegmentWorldMatrix, viewMatrix), projectionMatrix);
                    commandList->SetGraphicsRootConstantBufferView(
                        0,
                        attackEffectMaterialConstantBuffer.GetGPUVirtualAddress());
                    commandList->SetGraphicsRootConstantBufferView(
                        1,
                        circleTransformConstantBuffer.GetGPUVirtualAddress());
                    enemyAttackEffectModel_.Draw(commandList, &textureManager_);
                }
            }
            else if (isTelegraphVisible) {
                const bool isEightWay =
                    phase2AttackPattern_ == Phase2AttackPattern::EightWay;
                const float effectWidthScale = isTelegraph
                    ? kPhase2TelegraphWidthScale
                    : kPhase2AttackEffectWidthScale;
                const float effectLengthScale = isTelegraph
                    ? (isEightWay
                        ? kPhase2EightWayTelegraphLengthScale
                        : kPhase2AttackEffectScale)
                    : (isEightWay
                        ? kPhase2AttackEffectScale
                        : (isFastSpin
                            ? kPhase2FastSpinLengthScale
                            : kPhase2RainLengthScale));
                for (size_t effectIndex = 0; effectIndex < phase2AttackEffectActiveCount_; ++effectIndex) {
                    const Vector3& direction = phase2AttackEffectDirections_[effectIndex];
                    const Vector3& position = phase2AttackEffectPositions_[effectIndex];
                    Vector3 renderPosition = position;
                    if (isTelegraph && isEightWay && !enemyPositions_.empty()) {
                        const float telegraphHalfLength =
                            kPhase2AttackEffectModelHalfLength * effectLengthScale;
                        const float telegraphDistance =
                            telegraphHalfLength + kPhase2EightWayTelegraphCenterGap;
                        renderPosition = {
                            attackOrigin.x + direction.x * telegraphDistance,
                            attackOrigin.y + direction.y * telegraphDistance,
                            attackOrigin.z,
                        };
                    }

                    // Build the basis directly: the model's local +Y axis becomes
                    // the requested world direction. This avoids Euler rotation
                    // convention differences and guarantees eight radial spokes.
                    Matrix4x4 effectWorldMatrix = MakeIdentityMatrix();
                    effectWorldMatrix.m[0][0] = direction.y * effectWidthScale;
                    effectWorldMatrix.m[0][1] = -direction.x * effectWidthScale;
                    effectWorldMatrix.m[1][0] = direction.x * effectLengthScale;
                    effectWorldMatrix.m[1][1] = direction.y * effectLengthScale;
                    effectWorldMatrix.m[2][2] = effectWidthScale;
                    effectWorldMatrix.m[3][0] = renderPosition.x;
                    effectWorldMatrix.m[3][1] = renderPosition.y;
                    effectWorldMatrix.m[3][2] = kPhase2EffectRenderZ;
                    TransformationMatrixConstantBuffer& effectTransformConstantBuffer =
                        attackEffectTransformConstantBuffers[effectIndex];
                    effectTransformConstantBuffer->World = effectWorldMatrix;
                    effectTransformConstantBuffer->WVP = Multiply(
                        Multiply(effectWorldMatrix, viewMatrix), projectionMatrix);
                    commandList->SetGraphicsRootConstantBufferView(
                        0,
                        attackEffectMaterialConstantBuffer.GetGPUVirtualAddress());
                    commandList->SetGraphicsRootConstantBufferView(
                        1,
                        effectTransformConstantBuffer.GetGPUVirtualAddress());
                    enemyAttackEffectModel_.Draw(commandList, &textureManager_);
                }
            }
        }

        for (size_t projectileIndex = 0; projectileIndex < projectiles_.size(); ++projectileIndex) {
            const Projectile& projectile = projectiles_[projectileIndex];
            if (!projectile.isActive) {
                continue;
            }

            TransformationMatrixConstantBuffer& projectileTransformConstantBuffer =
                *projectileTransformConstantBuffers_[projectileIndex];
            const Matrix4x4 projectileWorldMatrix = MakeAffineMatrix(
                { projectile.scale, projectile.scale, projectile.scale },
                { 0.0f, 0.0f, 0.0f },
                projectile.position);
            projectileTransformConstantBuffer->World = projectileWorldMatrix;
            projectileTransformConstantBuffer->WVP = Multiply(
                Multiply(projectileWorldMatrix, viewMatrix), projectionMatrix);
            commandList->SetGraphicsRootConstantBufferView(
                0,
                projectileMaterialConstantBuffer_.GetGPUVirtualAddress());
            commandList->SetGraphicsRootConstantBufferView(
                1,
                projectileTransformConstantBuffer.GetGPUVirtualAddress());
            bulletModel_.Draw(commandList, &textureManager_);
        }

        if (isOrientationMarkerVisible_) {
            constexpr int32_t kMarkerSize = 128;
            constexpr int32_t kMarkerMargin = 16;
            D3D12_VIEWPORT markerViewport{};
            markerViewport.TopLeftX = static_cast<float>(kClientWidth - kMarkerMargin - kMarkerSize);
            markerViewport.TopLeftY = static_cast<float>(kMarkerMargin);
            markerViewport.Width = static_cast<float>(kMarkerSize);
            markerViewport.Height = static_cast<float>(kMarkerSize);
            markerViewport.MinDepth = 0.0f;
            markerViewport.MaxDepth = 1.0f;
            const D3D12_RECT markerScissorRect{
                static_cast<LONG>(markerViewport.TopLeftX),
                static_cast<LONG>(markerViewport.TopLeftY),
                static_cast<LONG>(markerViewport.TopLeftX + markerViewport.Width),
                static_cast<LONG>(markerViewport.TopLeftY + markerViewport.Height),
            };
            commandList->RSSetViewports(1, &markerViewport);
            commandList->RSSetScissorRects(1, &markerScissorRect);

            Matrix4x4 rotationOnlyView = viewMatrix;
            rotationOnlyView.m[3][0] = 0.0f;
            rotationOnlyView.m[3][1] = 0.0f;
            rotationOnlyView.m[3][2] = 0.0f;
            rotationOnlyView.m[3][3] = 1.0f;

            // Positive world axes: X is red, Y is green, and Z is blue.
            constexpr Vector3 kAxisScales[] = {
                { 3.0f, 0.08f, 0.08f },
                { 0.08f, 3.0f, 0.08f },
                { 0.08f, 0.08f, 3.0f },
            };
            constexpr Vector3 kAxisPositions[] = {
                { 3.0f, 0.0f, 0.0f },
                { 0.0f, 3.0f, 0.0f },
                { 0.0f, 0.0f, 3.0f },
            };
            for (size_t axisIndex = 0; axisIndex < axisTransformConstantBuffers_.size(); ++axisIndex) {
                TransformationMatrixConstantBuffer& axisTransform = axisTransformConstantBuffers_[axisIndex];
                const Matrix4x4 worldMatrix = MakeAffineMatrix(
                    kAxisScales[axisIndex],
                    { 0.0f, 0.0f, 0.0f },
                    kAxisPositions[axisIndex]);
                axisTransform->World = worldMatrix;
                axisTransform->WVP = Multiply(
                    Multiply(worldMatrix, rotationOnlyView),
                    MakeOrientationMarkerProjectionMatrix());
                commandList->SetGraphicsRootConstantBufferView(
                    0,
                    axisMaterialConstantBuffers_[axisIndex].GetGPUVirtualAddress());
                commandList->SetGraphicsRootConstantBufferView(1, axisTransform.GetGPUVirtualAddress());
                cubeModel_.Draw(commandList, &textureManager_);
            }
        }

        D3D12_VIEWPORT sceneViewport{};
        sceneViewport.Width = static_cast<float>(kClientWidth);
        sceneViewport.Height = static_cast<float>(kClientHeight);
        sceneViewport.MinDepth = 0.0f;
        sceneViewport.MaxDepth = 1.0f;
        const D3D12_RECT sceneScissorRect{ 0, 0, kClientWidth, kClientHeight };
        commandList->RSSetViewports(1, &sceneViewport);
        commandList->RSSetScissorRects(1, &sceneScissorRect);

        const Matrix4x4 screenSpaceProjection = MakeScreenSpaceProjectionMatrix();

        // The combined UI image is the fixed gauge background. It always
        // remains at its original 1280x720 size regardless of current HP.
        uiSprite_.Update(screenSpaceProjection);
        uiSprite_.Draw(commandList, nullptr);

        if (showGameTimer) {
            const uint32_t totalSeconds = static_cast<uint32_t>((std::max)(0.0f, elapsedGameTimeSeconds));
            const uint32_t minutes = (std::min)(totalSeconds / 60, 99u);
            const uint32_t seconds = totalSeconds % 60;
            const uint32_t timerDigits[] = {
                minutes / 10,
                minutes % 10,
                seconds / 10,
                seconds % 10,
            };
            for (size_t digitIndex = 0; digitIndex < timerDigitSprites_.size(); ++digitIndex) {
                timerDigitSprites_[digitIndex].SetTextureSrvHandle(
                    timerDigitTextureHandles_[timerDigits[digitIndex]]);
                timerDigitSprites_[digitIndex].Update(screenSpaceProjection);
                timerDigitSprites_[digitIndex].Draw(commandList, nullptr);
            }
            timerColonSprite_.Update(screenSpaceProjection);
            timerColonSprite_.Draw(commandList, nullptr);
        }

        int32_t displayedProjectileCount = GetRemainingProjectileCount();
        if (isPlayerCharging_) {
            constexpr float kChargeSecondsPerAmmo = 0.25f;
            const int32_t chargingAmmoCost = (std::min)(
                static_cast<int32_t>(playerChargeTime_ / kChargeSecondsPerAmmo) + 1,
                displayedProjectileCount);
            displayedProjectileCount -= chargingAmmoCost;
        }
        for (int32_t bulletIndex = 0; bulletIndex < displayedProjectileCount; ++bulletIndex) {
            bulletCountSprites_[static_cast<size_t>(bulletIndex)].Update(screenSpaceProjection);
            bulletCountSprites_[static_cast<size_t>(bulletIndex)].Draw(commandList, nullptr);
        }

        if (!enemyHealths_.empty()) {
            const float enemyHealthRate = std::clamp(
                enemyHealths_[0] / GetEnemyMaxHealth(),
                0.0f,
                1.0f);
            if (enemyHealthRate > 0.0f) {
                // Keep the original 1280x720 image size and reveal only the
                // remaining right-hand portion of the enemy HP bar.
                const LONG enemyBarRight = static_cast<LONG>(
                    kEnemyHpBarPosition.x + static_cast<float>(kHpBarWidth));
                const D3D12_RECT enemyHpScissor{
                    enemyBarRight - static_cast<LONG>(
                        static_cast<float>(kHpBarWidth) * enemyHealthRate),
                    static_cast<LONG>(kEnemyHpBarPosition.y),
                    enemyBarRight,
                    static_cast<LONG>(kEnemyHpBarPosition.y + static_cast<float>(kHpBarHeight)),
                };
                commandList->RSSetScissorRects(1, &enemyHpScissor);
                enemyHpSprite_.Update(screenSpaceProjection);
                enemyHpSprite_.Draw(commandList, nullptr);
            }
        }

        const float playerHealthRate = std::clamp(
            playerHealth_ / GetPlayerMaxHealth(),
            0.0f,
            1.0f);
        if (playerHealthRate > 0.0f) {
            // Keep the original 1280x720 image size and reveal only the
            // remaining left-hand portion of the player HP bar.
            const D3D12_RECT playerHpScissor{
                static_cast<LONG>(kPlayerHpBarPosition.x),
                static_cast<LONG>(kPlayerHpBarPosition.y),
                static_cast<LONG>(kPlayerHpBarPosition.x +
                    static_cast<float>(kHpBarWidth) * playerHealthRate),
                static_cast<LONG>(kPlayerHpBarPosition.y + static_cast<float>(kHpBarHeight)),
            };
            commandList->RSSetScissorRects(1, &playerHpScissor);
            playerHpSprite_.Update(screenSpaceProjection);
            playerHpSprite_.Draw(commandList, nullptr);
        }

        commandList->RSSetScissorRects(1, &sceneScissorRect);
        if (isTutorialMode_) {
            const size_t currentStepIndex =
                static_cast<size_t>(tutorialStep_);
            const float pulseScale = 1.0f +
                0.10f * std::sin(tutorialIconAnimationTime_ * 6.0f);
            for (size_t buttonIndex = 0;
                buttonIndex < tutorialButtonSprites_.size();
                ++buttonIndex) {
                const size_t buttonStepIndex = buttonIndex < 4
                    ? buttonIndex
                    : static_cast<size_t>(TutorialStep::ChargeAttack);
                const bool isCurrentStep = buttonStepIndex == currentStepIndex;
                const bool isCompletedStep =
                    tutorialStep_ == TutorialStep::Complete ||
                    buttonStepIndex < currentStepIndex;
                const float buttonScale = isCurrentStep ? pulseScale : 1.0f;
                const float buttonOpacity = isCurrentStep
                    ? 1.0f
                    : (isCompletedStep ? 0.60f : 0.25f);
                const size_t buttonGroupIndex = buttonIndex < 4
                    ? buttonIndex
                    : 3;
                const float baseLeft = kTutorialButtonLeft +
                    static_cast<float>(buttonGroupIndex) *
                    (static_cast<float>(kTutorialButtonSize) +
                        kTutorialButtonSpacing);
                float horizontalScale = buttonScale;
                if (buttonIndex == 3 && isCurrentStep) {
                    const float actualHoldProgress = std::clamp(
                        playerChargeTime_ / kTutorialFullChargeDuration,
                        0.0f,
                        1.0f);
                    const float remainingGauge = 1.0f - actualHoldProgress;
                    horizontalScale *= remainingGauge;
                    horizontalScale = (std::max)(horizontalScale, 0.05f);
                }
                tutorialButtonSprites_[buttonIndex].transform.scale = {
                    horizontalScale,
                    buttonScale,
                    1.0f,
                };
                tutorialButtonSprites_[buttonIndex].transform.translate = {
                    baseLeft - static_cast<float>(kTutorialButtonSize) *
                        (buttonScale - 1.0f) * 0.5f,
                    kTutorialButtonTop - static_cast<float>(kTutorialButtonSize) *
                        (buttonScale - 1.0f) * 0.5f,
                    0.0f,
                };
                tutorialButtonSprites_[buttonIndex].color = {
                    1.0f, 1.0f, 1.0f, buttonOpacity,
                };
                tutorialButtonSprites_[buttonIndex].Update(screenSpaceProjection);
                tutorialButtonSprites_[buttonIndex].Draw(commandList, nullptr);
            }
            {
                const float startScale = tutorialStep_ == TutorialStep::Complete
                    ? pulseScale
                    : 1.0f;
                const float startOpacity =
                    tutorialStep_ == TutorialStep::Complete ? 1.0f : 0.25f;
                const float startDisplayLeft =
                    static_cast<float>(kUiTextureWidth) -
                    kTutorialStartDisplayMargin -
                    kTutorialStartDisplayWidth;
                const float startDisplayTop =
                    static_cast<float>(kUiTextureHeight) -
                    kTutorialStartDisplayMargin -
                    static_cast<float>(kTutorialButtonSize);
                tutorialStartButtonSprite_.transform.scale = {
                    startScale, startScale, 1.0f,
                };
                tutorialStartButtonSprite_.transform.translate = {
                    startDisplayLeft - static_cast<float>(kTutorialButtonSize) *
                        (startScale - 1.0f) * 0.5f,
                    startDisplayTop - static_cast<float>(kTutorialButtonSize) *
                        (startScale - 1.0f) * 0.5f,
                    0.0f,
                };
                tutorialStartButtonSprite_.color = {
                    1.0f, 1.0f, 1.0f, startOpacity,
                };
                tutorialStartButtonSprite_.Update(screenSpaceProjection);
                tutorialStartButtonSprite_.Draw(commandList, nullptr);

                const float skipGaugeHorizontalScale = (std::max)(
                    tutorialSkipHoldProgress_ * startScale,
                    0.01f);
                tutorialSkipGaugeSprite_.transform.scale = {
                    skipGaugeHorizontalScale,
                    startScale,
                    1.0f,
                };
                tutorialSkipGaugeSprite_.transform.translate = {
                    startDisplayLeft - static_cast<float>(kTutorialButtonSize) *
                        (startScale - 1.0f) * 0.5f,
                    startDisplayTop - static_cast<float>(kTutorialButtonSize) *
                        (startScale - 1.0f) * 0.5f,
                    0.0f,
                };
                tutorialSkipGaugeSprite_.color = {
                    1.0f,
                    1.0f,
                    1.0f,
                    tutorialSkipHoldProgress_ > 0.0f ? 1.0f : 0.0f,
                };
                tutorialSkipGaugeSprite_.Update(screenSpaceProjection);
                tutorialSkipGaugeSprite_.Draw(commandList, nullptr);

                tutorialStartArrowSprite_.transform.scale = {
                    startScale, startScale, 1.0f,
                };
                tutorialStartArrowSprite_.transform.translate = {
                    startDisplayLeft + static_cast<float>(kTutorialButtonSize) +
                        kTutorialStartDisplaySpacing -
                        static_cast<float>(kTutorialButtonSize) *
                        (startScale - 1.0f) * 0.5f,
                    startDisplayTop - static_cast<float>(kTutorialButtonSize) *
                        (startScale - 1.0f) * 0.5f,
                    0.0f,
                };
                tutorialStartArrowSprite_.color = {
                    1.0f, 1.0f, 1.0f, startOpacity,
                };
                tutorialStartArrowSprite_.Update(screenSpaceProjection);
                tutorialStartArrowSprite_.Draw(commandList, nullptr);
            }
        }
    }

    void GameScene::DrawTitleScreen(
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
        commandList->SetGraphicsRootConstantBufferView(
            2,
            lightConstantBuffer_.GetGPUVirtualAddress());
        const Matrix4x4 screenSpaceProjection = MakeScreenSpaceProjectionMatrix();
        const float resultStartDisplayLeft =
            static_cast<float>(kUiTextureWidth) -
            kTutorialStartDisplayMargin -
            kTutorialStartDisplayWidth;
        const float resultStartDisplayTop =
            static_cast<float>(kUiTextureHeight) -
            kTutorialStartDisplayMargin -
            static_cast<float>(kTutorialButtonSize);
        tutorialStartButtonSprite_.transform.scale = { 1.0f, 1.0f, 1.0f };
        tutorialStartButtonSprite_.transform.translate = {
            resultStartDisplayLeft,
            resultStartDisplayTop,
            0.0f,
        };
        tutorialStartButtonSprite_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        tutorialStartButtonSprite_.Update(screenSpaceProjection);
        tutorialStartButtonSprite_.Draw(commandList, nullptr);

        tutorialStartArrowSprite_.transform.scale = { 1.0f, 1.0f, 1.0f };
        tutorialStartArrowSprite_.transform.translate = {
            resultStartDisplayLeft + static_cast<float>(kTutorialButtonSize) +
                kTutorialStartDisplaySpacing,
            resultStartDisplayTop,
            0.0f,
        };
        tutorialStartArrowSprite_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        tutorialStartArrowSprite_.Update(screenSpaceProjection);
        tutorialStartArrowSprite_.Draw(commandList, nullptr);
    }

    void GameScene::DrawTitleOverlay(
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
        commandList->SetGraphicsRootConstantBufferView(
            2,
            lightConstantBuffer_.GetGPUVirtualAddress());
        const Matrix4x4 screenSpaceProjection =
            MakeScreenSpaceProjectionMatrix();

        titleControllerSprite_.transform.scale = { 1.0f, 1.0f, 1.0f };
        titleControllerSprite_.transform.translate = { 0.0f, 0.0f, 0.0f };
        titleControllerSprite_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        titleControllerSprite_.Update(screenSpaceProjection);
        titleControllerSprite_.Draw(commandList, nullptr);

        titleLogoSprite_.transform.scale = { 1.0f, 1.0f, 1.0f };
        titleLogoSprite_.transform.translate = { 0.0f, 0.0f, 0.0f };
        titleLogoSprite_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        titleLogoSprite_.Update(screenSpaceProjection);
        titleLogoSprite_.Draw(commandList, nullptr);

        if (isNightmareMode_) {
            titleNightmareSprite_.transform.scale = { 1.0f, 1.0f, 1.0f };
            titleNightmareSprite_.transform.translate = {
                0.0f, 0.0f, 0.0f,
            };
            titleNightmareSprite_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            titleNightmareSprite_.Update(screenSpaceProjection);
            titleNightmareSprite_.Draw(commandList, nullptr);
        }

        constexpr float kGameStartBobAmplitude = 8.0f;
        constexpr float kGameStartBobSpeed = 2.0f;
        const float gameStartBobOffset = std::sin(
            titleAnimationTime_ * kGameStartBobSpeed) *
            kGameStartBobAmplitude;
        titleGameStartSprite_.transform.scale = { 1.0f, 1.0f, 1.0f };
        titleGameStartSprite_.transform.translate = {
            0.0f,
            gameStartBobOffset,
            0.0f,
        };
        titleGameStartSprite_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        titleGameStartSprite_.Update(screenSpaceProjection);
        titleGameStartSprite_.Draw(commandList, nullptr);

        constexpr float kTitleAButtonLeft =
            (static_cast<float>(kUiTextureWidth) -
                static_cast<float>(kTutorialButtonSize)) * 0.5f;
        constexpr float kTitleAButtonTop = 575.0f;
        Sprite& titleAButtonSprite = tutorialButtonSprites_[1];
        titleAButtonSprite.transform.scale = { 1.0f, 1.0f, 1.0f };
        titleAButtonSprite.transform.translate = {
            kTitleAButtonLeft,
            kTitleAButtonTop,
            0.0f,
        };
        titleAButtonSprite.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        titleAButtonSprite.Update(screenSpaceProjection);
        titleAButtonSprite.Draw(commandList, nullptr);

        titleToRankingSprite_.transform.scale = { 1.0f, 1.0f, 1.0f };
        titleToRankingSprite_.transform.translate = { 0.0f, 0.0f, 0.0f };
        titleToRankingSprite_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        titleToRankingSprite_.Update(screenSpaceProjection);
        titleToRankingSprite_.Draw(commandList, nullptr);

        Sprite& titleRankingRtSprite = tutorialButtonSprites_[2];
        titleRankingRtSprite.transform.scale = { 0.8f, 0.8f, 1.0f };
        titleRankingRtSprite.transform.translate = { 16.0f, 648.0f, 0.0f };
        titleRankingRtSprite.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        titleRankingRtSprite.Update(screenSpaceProjection);
        titleRankingRtSprite.Draw(commandList, nullptr);
    }

    void GameScene::DrawResultScreen(
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
        float elapsedGameTimeSeconds,
        bool isClear,
        bool showGoNightmareTip) {
        const Matrix4x4 screenSpaceProjection = MakeScreenSpaceProjectionMatrix();
        const bool shouldShowGoNightmareTip =
            isClear && showGoNightmareTip;
        if (!shouldShowGoNightmareTip) {
            Sprite& resultBackgroundSprite = isClear
                ? gameClearSprite_
                : gameOverSprite_;
            resultBackgroundSprite.Update(screenSpaceProjection);
            resultBackgroundSprite.Draw(commandList, nullptr);
        }

        if (shouldShowGoNightmareTip) {
            goNightmareSprite_.transform.scale = { 1.0f, 1.0f, 1.0f };
            goNightmareSprite_.transform.translate = { 0.0f, 0.0f, 0.0f };
            goNightmareSprite_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            goNightmareSprite_.Update(screenSpaceProjection);
            goNightmareSprite_.Draw(commandList, nullptr);

            goNightmareStartButtonSprite_.transform.scale = {
                1.0f, 1.0f, 1.0f,
            };
            goNightmareStartButtonSprite_.transform.translate = {
                176.0f,
                480.0f,
                0.0f,
            };
            goNightmareStartButtonSprite_.color = {
                1.0f, 1.0f, 1.0f, 1.0f,
            };
            goNightmareStartButtonSprite_.Update(screenSpaceProjection);
            goNightmareStartButtonSprite_.Draw(commandList, nullptr);
        }

        DrawTitleScreen(commandList);

        if (!isClear || shouldShowGoNightmareTip) {
            return;
        }

        constexpr uint64_t kMaximumResultCentiseconds =
            99ull * 60ull * 100ull + 59ull * 100ull + 99ull;
        const uint64_t totalCentiseconds = (std::min)(
            static_cast<uint64_t>((std::max)(0.0f, elapsedGameTimeSeconds) * 100.0f),
            kMaximumResultCentiseconds);
        const uint32_t minutes = static_cast<uint32_t>(
            totalCentiseconds / (60ull * 100ull));
        const uint32_t seconds = static_cast<uint32_t>(
            (totalCentiseconds / 100ull) % 60ull);
        const uint32_t centiseconds = static_cast<uint32_t>(
            totalCentiseconds % 100ull);
        const uint32_t resultDigits[] = {
            minutes / 10,
            minutes % 10,
            seconds / 10,
            seconds % 10,
            centiseconds / 10,
            centiseconds % 10,
        };

        for (size_t digitIndex = 0;
            digitIndex < resultTimeDigitSprites_.size();
            ++digitIndex) {
            resultTimeDigitSprites_[digitIndex].SetTextureSrvHandle(
                timerDigitTextureHandles_[resultDigits[digitIndex]]);
            resultTimeDigitSprites_[digitIndex].Update(screenSpaceProjection);
            resultTimeDigitSprites_[digitIndex].Draw(commandList, nullptr);
        }
        for (Sprite& colonSprite : resultTimeColonSprites_) {
            colonSprite.Update(screenSpaceProjection);
            colonSprite.Draw(commandList, nullptr);
        }
    }

    void GameScene::DrawRankingScreen(
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
        commandList->SetGraphicsRootConstantBufferView(
            2,
            lightConstantBuffer_.GetGPUVirtualAddress());
        const Matrix4x4 screenSpaceProjection =
            MakeScreenSpaceProjectionMatrix();
        Sprite& rankingBackgroundSprite = isNightmareMode_
            ? nightmareRankingBackgroundSprite_
            : rankingBackgroundSprite_;
        rankingBackgroundSprite.transform.scale = { 1.0f, 1.0f, 1.0f };
        rankingBackgroundSprite.transform.translate = { 0.0f, 0.0f, 0.0f };
        rankingBackgroundSprite.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        rankingBackgroundSprite.Update(screenSpaceProjection);
        rankingBackgroundSprite.Draw(commandList, nullptr);

        constexpr uint64_t kMaximumRankingCentiseconds =
            99ull * 60ull * 100ull + 59ull * 100ull + 99ull;
        for (size_t rankIndex = 0;
            rankIndex < clearRankingCount_;
            ++rankIndex) {
            const uint64_t rankingCentiseconds = (std::min)(
                static_cast<uint64_t>((std::max)(
                    0.0f,
                    clearRankingTimes_[rankIndex]) * 100.0f),
                kMaximumRankingCentiseconds);
            const uint32_t rankingMinutes = static_cast<uint32_t>(
                rankingCentiseconds / (60ull * 100ull));
            const uint32_t rankingSeconds = static_cast<uint32_t>(
                (rankingCentiseconds / 100ull) % 60ull);
            const uint32_t rankingFraction = static_cast<uint32_t>(
                rankingCentiseconds % 100ull);
            const std::array<uint32_t, 6> rankingDigits = {
                rankingMinutes / 10,
                rankingMinutes % 10,
                rankingSeconds / 10,
                rankingSeconds % 10,
                rankingFraction / 10,
                rankingFraction % 10,
            };

            for (size_t digitIndex = 0;
                digitIndex < rankingDigits.size();
                ++digitIndex) {
                Sprite& digitSprite =
                    resultRankingDigitSprites_[rankIndex][digitIndex];
                digitSprite.SetTextureSrvHandle(
                    timerDigitTextureHandles_[rankingDigits[digitIndex]]);
                digitSprite.color = { 1.0f, 1.0f, 1.0f, 1.0f };
                digitSprite.Update(screenSpaceProjection);
                digitSprite.Draw(commandList, nullptr);
            }
            for (Sprite& colonSprite :
                resultRankingColonSprites_[rankIndex]) {
                colonSprite.color = { 1.0f, 1.0f, 1.0f, 1.0f };
                colonSprite.Update(screenSpaceProjection);
                colonSprite.Draw(commandList, nullptr);
            }
        }

        constexpr float kRankingCloseSpacing = 8.0f;
        constexpr float kRankingCloseMargin = 32.0f;
        constexpr float kRankingCloseWidth =
            static_cast<float>(kTutorialButtonSize) * 2.0f +
            kRankingCloseSpacing;
        const float rankingCloseLeft =
            static_cast<float>(kUiTextureWidth) -
            kRankingCloseMargin - kRankingCloseWidth;
        const float rankingCloseTop =
            static_cast<float>(kUiTextureHeight) -
            kRankingCloseMargin -
            static_cast<float>(kTutorialButtonSize);

        Sprite& rankingCloseRtSprite = tutorialButtonSprites_[2];
        rankingCloseRtSprite.transform.scale = { 1.0f, 1.0f, 1.0f };
        rankingCloseRtSprite.transform.translate = {
            rankingCloseLeft,
            rankingCloseTop,
            0.0f,
        };
        rankingCloseRtSprite.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        rankingCloseRtSprite.Update(screenSpaceProjection);
        rankingCloseRtSprite.Draw(commandList, nullptr);

        tutorialStartArrowSprite_.transform.scale = { 1.0f, 1.0f, 1.0f };
        tutorialStartArrowSprite_.transform.translate = {
            rankingCloseLeft + static_cast<float>(kTutorialButtonSize) +
                kRankingCloseSpacing,
            rankingCloseTop,
            0.0f,
        };
        tutorialStartArrowSprite_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        tutorialStartArrowSprite_.Update(screenSpaceProjection);
        tutorialStartArrowSprite_.Draw(commandList, nullptr);
    }

}
