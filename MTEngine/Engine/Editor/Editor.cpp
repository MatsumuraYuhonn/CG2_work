#include "Editor.h"
#include "MTEngine/Engine/Debug/DebugCamera.h"
#include "MTEngine/Game/Scene/GameScene.h"
#include "MTEngine/Game/Scene/SceneManager.h"

#include <cstdio>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_internal.h"
#endif

namespace MTEngine {

    void Editor::Initialize(
        D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle,
        GameScene* gameScene,
        DebugCamera* debugCamera,
        SceneManager* sceneManager) {
        sceneTextureHandle_ = sceneTextureHandle;
        gameScene_ = gameScene;
        debugCamera_ = debugCamera;
        sceneManager_ = sceneManager;
    }

    void Editor::Update() {
#ifdef USE_IMGUI
        if (!isLayoutInitialized_) {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            const ImGuiID dockspaceID = ImGui::GetID("MainDockSpace");
            ImGui::DockBuilderRemoveNode(dockspaceID);
            ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceID, viewport->WorkSize);

            ImGuiID centerNode = dockspaceID;
            ImGuiID rightNode = 0;
            ImGuiID rightBottomNode = 0;
            ImGuiID leftNode = 0;
            ImGuiID bottomNode = 0;
            ImGui::DockBuilderSplitNode(centerNode, ImGuiDir_Right, 0.25f, &rightNode, &centerNode);
            ImGui::DockBuilderSplitNode(rightNode, ImGuiDir_Down, 0.50f, &rightBottomNode, &rightNode);
            ImGui::DockBuilderSplitNode(centerNode, ImGuiDir_Left, 0.20f, &leftNode, &centerNode);
            ImGui::DockBuilderSplitNode(centerNode, ImGuiDir_Down, 0.30f, &bottomNode, &centerNode);
            ImGui::DockBuilderDockWindow("Hierarchy", leftNode);
            ImGui::DockBuilderDockWindow("Inspector", rightNode);
            ImGui::DockBuilderDockWindow("Inspector 2", rightBottomNode);
            ImGui::DockBuilderDockWindow("Console", bottomNode);
            ImGui::DockBuilderDockWindow("Scene", centerNode);
            ImGui::DockBuilderFinish(dockspaceID);
            isLayoutInitialized_ = true;
        }

        constexpr ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoCollapse;

        ImGui::Begin("Hierarchy", nullptr, panelFlags);
        ImGui::TextDisabled("Scene Objects");
        ImGui::Separator();
        if (debugCamera_ && ImGui::Selectable("Debug Camera", isDebugCameraSelected_)) {
            isDebugCameraSelected_ = true;
            isPlayerSelected_ = false;
            selectedTileIndex_ = -1;
            selectedEnemyIndex_ = -1;
        }
        if (!gameScene_) {
            ImGui::TextDisabled("No map loaded");
        }
        else {
            const TileMap& tileMap = gameScene_->GetTileMap();
            const std::vector<TileMapCell>& cells = gameScene_->GetCells();
            ImGui::Text("Tile Map (%d x %d)", tileMap.GetWidth(), tileMap.GetHeight());
            if (ImGui::TreeNode("Tiles")) {
                for (size_t tileIndex = 0; tileIndex < cells.size(); ++tileIndex) {
                    const TileMapCell& cell = cells[tileIndex];
                    char label[64]{};
                    std::snprintf(label, sizeof(label), "Tile %d, %d", cell.column, cell.row);
                    if (ImGui::Selectable(label, selectedTileIndex_ == static_cast<int32_t>(tileIndex))) {
                        selectedTileIndex_ = static_cast<int32_t>(tileIndex);
                        isDebugCameraSelected_ = false;
                        isPlayerSelected_ = false;
                        selectedEnemyIndex_ = -1;
                    }
                }
                ImGui::TreePop();
            }
            if (ImGui::Selectable("Player", isPlayerSelected_)) {
                isPlayerSelected_ = true;
                isDebugCameraSelected_ = false;
                selectedTileIndex_ = -1;
                selectedEnemyIndex_ = -1;
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Middle)) {
                secondaryInspectorTarget_ = SecondaryInspectorTarget::Player;
                secondaryEnemyIndex_ = -1;
            }
            const std::vector<Vector3>& enemyPositions = gameScene_->GetEnemyPositions();
            for (size_t enemyIndex = 0; enemyIndex < enemyPositions.size(); ++enemyIndex) {
                char label[32]{};
                std::snprintf(label, sizeof(label), "Enemy %zu", enemyIndex + 1);
                if (ImGui::Selectable(label, selectedEnemyIndex_ == static_cast<int32_t>(enemyIndex))) {
                    selectedEnemyIndex_ = static_cast<int32_t>(enemyIndex);
                    isDebugCameraSelected_ = false;
                    isPlayerSelected_ = false;
                    selectedTileIndex_ = -1;
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Middle)) {
                    secondaryInspectorTarget_ = SecondaryInspectorTarget::Enemy;
                    secondaryEnemyIndex_ = static_cast<int32_t>(enemyIndex);
                }
            }
        }
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Scene", nullptr, panelFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        if (sceneManager_) {
            constexpr const char* sceneNames[] = { "Title", "Tutorial", "Game", "Clear", "Miss" };
            int32_t selectedScene = static_cast<int32_t>(sceneManager_->GetCurrentScene());
            ImGui::SetNextItemWidth(160.0f);
            if (ImGui::Combo("Current Scene", &selectedScene, sceneNames, IM_ARRAYSIZE(sceneNames)) && gameScene_) {
                sceneManager_->ChangeScene(static_cast<SceneType>(selectedScene), *gameScene_);
            }
            if (sceneManager_->IsGameScene() && gameScene_) {
                constexpr const char* phaseNames[] = { "Phase 1", "Phase 2" };
                int32_t selectedPhase = gameScene_->GetBossPhase() - 1;
                ImGui::SetNextItemWidth(160.0f);
                if (ImGui::Combo("Boss Phase", &selectedPhase, phaseNames, IM_ARRAYSIZE(phaseNames))) {
                    gameScene_->SetBossPhase(selectedPhase + 1);
                }
            }
            ImGui::TextDisabled("%s", sceneManager_->GetTransitionCondition());
            if (sceneManager_->GetCurrentScene() == SceneType::Tutorial) {
                ImGui::Spacing();
                ImGui::Text("Tutorial");
                ImGui::BulletText("Move: A / D or Left Stick");
                ImGui::BulletText("Jump: W or GamePad A");
                ImGui::BulletText("Charge attack: Hold Space or GamePad RT");
                ImGui::BulletText("Release the button to fire");
                ImGui::TextColored(
                    ImVec4(0.95f, 0.80f, 0.25f, 1.0f),
                    "Press Enter or GamePad Start to begin");
            }
            ImGui::Separator();
        }
        const ImVec2 sceneSize = ImGui::GetContentRegionAvail();
        ImGui::Image(ImTextureRef(static_cast<ImTextureID>(sceneTextureHandle_.ptr)), sceneSize);
        isSceneViewFocused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Begin("Inspector", nullptr, panelFlags);
        if (isDebugCameraSelected_ && debugCamera_) {
            Vector3 position = debugCamera_->GetPosition();
            Vector3 target = debugCamera_->GetTarget();
            float distance = debugCamera_->GetDistance();
            float yaw = debugCamera_->GetYaw();
            float pitch = debugCamera_->GetPitch();
            ImGui::Text("Debug Camera");
            ImGui::Separator();
            ImGui::Text("Position");
            if (ImGui::DragFloat3("##Position", &position.x, 0.1f)) {
                debugCamera_->SetPosition(position);
            }
            ImGui::Separator();
            ImGui::Text("Target");
            if (ImGui::DragFloat3("##Target", &target.x, 0.1f)) {
                debugCamera_->SetTarget(target);
            }
            ImGui::Separator();
            if (ImGui::DragFloat("Distance", &distance, 0.1f, 0.1f, 1000.0f, "%.2f")) {
                debugCamera_->SetDistance(distance);
            }
            if (ImGui::SliderAngle("Yaw", &yaw, -360.0f, 360.0f)) {
                debugCamera_->SetYaw(yaw);
            }
            if (ImGui::SliderAngle("Pitch", &pitch, -89.0f, 89.0f)) {
                debugCamera_->SetPitch(pitch);
            }
        }
        else if (isPlayerSelected_ && gameScene_) {
            Vector3 playerPosition = gameScene_->GetPlayerPosition();
            ImGui::Text("Player");
            ImGui::Separator();
            ImGui::Text("World Position");
            const bool positionChanged =
                ImGui::SliderFloat("Position X", &playerPosition.x, -30.0f, 30.0f, "%.2f") |
                ImGui::SliderFloat("Position Y", &playerPosition.y, -20.0f, 20.0f, "%.2f") |
                ImGui::SliderFloat("Position Z", &playerPosition.z, -5.0f, 5.0f, "%.2f");
            if (positionChanged) {
                gameScene_->SetPlayerPosition(playerPosition);
            }
            ImGui::Separator();
            float acceleration = gameScene_->GetPlayerMoveAcceleration();
            float damping = gameScene_->GetPlayerVelocityDamping();
            float maxMoveSpeed = gameScene_->GetPlayerMaxMoveSpeed();
            float jumpSpeed = gameScene_->GetPlayerJumpSpeed();
            float health = gameScene_->GetPlayerHealth();
            if (ImGui::SliderFloat(
                "HP",
                &health,
                0.0f,
                gameScene_->GetPlayerMaxHealth(),
                "%.0f")) {
                gameScene_->SetPlayerHealth(health);
            }
            ImGui::Separator();
            if (ImGui::SliderFloat("Acceleration", &acceleration, 0.0f, 50.0f, "%.1f")) {
                gameScene_->SetPlayerMoveAcceleration(acceleration);
            }
            if (ImGui::SliderFloat("Velocity Damping", &damping, 0.0f, 20.0f, "%.1f")) {
                gameScene_->SetPlayerVelocityDamping(damping);
            }
            if (ImGui::SliderFloat("Max Move Speed", &maxMoveSpeed, 0.0f, 20.0f, "%.1f")) {
                gameScene_->SetPlayerMaxMoveSpeed(maxMoveSpeed);
            }
            if (ImGui::SliderFloat("Jump Speed", &jumpSpeed, 0.0f, 30.0f, "%.1f")) {
                gameScene_->SetPlayerJumpSpeed(jumpSpeed);
            }
            ImGui::Text("Horizontal Velocity: %.2f", gameScene_->GetPlayerHorizontalVelocity());
            ImGui::Text("Vertical Velocity: %.2f", gameScene_->GetPlayerVerticalVelocity());
            ImGui::Text("Grounded: %s", gameScene_->IsPlayerGrounded() ? "Yes" : "No");
        }
        else if (selectedEnemyIndex_ >= 0 && gameScene_) {
            const std::vector<Vector3>& enemyPositions = gameScene_->GetEnemyPositions();
            if (selectedEnemyIndex_ < static_cast<int32_t>(enemyPositions.size())) {
                Vector3 enemyPosition = enemyPositions[static_cast<size_t>(selectedEnemyIndex_)];
                float enemyHealth = gameScene_->GetEnemyHealth(static_cast<size_t>(selectedEnemyIndex_));
                ImGui::Text("Enemy %d", selectedEnemyIndex_ + 1);
                ImGui::Separator();
                if (ImGui::SliderFloat(
                    "HP", &enemyHealth, 0.0f, gameScene_->GetEnemyMaxHealth(), "%.0f")) {
                    gameScene_->SetEnemyHealth(static_cast<size_t>(selectedEnemyIndex_), enemyHealth);
                }
                ImGui::Separator();
                ImGui::Text("World Position");
                const bool positionChanged =
                    ImGui::SliderFloat("Position X", &enemyPosition.x, -30.0f, 30.0f, "%.2f") |
                    ImGui::SliderFloat("Position Y", &enemyPosition.y, -20.0f, 20.0f, "%.2f") |
                    ImGui::SliderFloat("Position Z", &enemyPosition.z, -5.0f, 5.0f, "%.2f");
                if (positionChanged) {
                    gameScene_->SetEnemyPosition(static_cast<size_t>(selectedEnemyIndex_), enemyPosition);
                }
                if (gameScene_->GetBossPhase() == 2) {
                    ImGui::Separator();
                    ImGui::Text("Boss Attacks");
                    if (ImGui::Button("Spawn 8-Way Attack")) {
                        gameScene_->SpawnPhase2AttackEffects();
                    }
                    if (ImGui::Button("Spawn Rain Attack")) {
                        gameScene_->SpawnPhase2RainAttack();
                    }
                    if (ImGui::Button("Spawn Fast Spin Attack")) {
                        gameScene_->SpawnPhase2FastSpinAttack();
                    }
                    if (ImGui::Button("Spawn Side Sweep Attack")) {
                        gameScene_->SpawnPhase2SideSweepAttack();
                    }
                }
            }
            else {
                selectedEnemyIndex_ = -1;
                ImGui::TextDisabled("No enemy selected");
            }
        }
        else if (!gameScene_) {
            ImGui::TextDisabled("No map loaded");
        }
        else {
            const std::vector<TileMapCell>& cells = gameScene_->GetCells();
            if (selectedTileIndex_ < 0 || selectedTileIndex_ >= static_cast<int32_t>(cells.size())) {
                ImGui::TextDisabled("No tile selected");
            }
            else {
                const TileMapCell& cell = cells[static_cast<size_t>(selectedTileIndex_)];
                ImGui::Text("Tile");
                ImGui::Separator();
                ImGui::Text("Tile ID: %d", cell.tileId);
                ImGui::Text("CSV column: %d", cell.column);
                ImGui::Text("CSV row: %d", cell.row);
                ImGui::Separator();
                ImGui::Text("World Position");
                ImGui::Text("X: %.2f", cell.worldPosition.x);
                ImGui::Text("Y: %.2f", cell.worldPosition.y);
                ImGui::Text("Z: %.2f", cell.worldPosition.z);
            }
        }
        ImGui::End();

        if (secondaryInspectorTarget_ != SecondaryInspectorTarget::None) {
            bool isSecondaryInspectorOpen = true;
            ImGui::Begin("Inspector 2", &isSecondaryInspectorOpen, panelFlags);
            if (secondaryInspectorTarget_ == SecondaryInspectorTarget::Player && gameScene_) {
                Vector3 playerPosition = gameScene_->GetPlayerPosition();
                float health = gameScene_->GetPlayerHealth();
                ImGui::Text("Player");
                ImGui::Separator();
                if (ImGui::SliderFloat(
                    "HP",
                    &health,
                    0.0f,
                    gameScene_->GetPlayerMaxHealth(),
                    "%.0f")) {
                    gameScene_->SetPlayerHealth(health);
                }
                ImGui::Text("World Position");
                const bool positionChanged =
                    ImGui::SliderFloat("Position X", &playerPosition.x, -30.0f, 30.0f, "%.2f") |
                    ImGui::SliderFloat("Position Y", &playerPosition.y, -20.0f, 20.0f, "%.2f") |
                    ImGui::SliderFloat("Position Z", &playerPosition.z, -5.0f, 5.0f, "%.2f");
                if (positionChanged) {
                    gameScene_->SetPlayerPosition(playerPosition);
                }
                ImGui::Separator();
                ImGui::Text("Horizontal Velocity: %.2f", gameScene_->GetPlayerHorizontalVelocity());
                ImGui::Text("Vertical Velocity: %.2f", gameScene_->GetPlayerVerticalVelocity());
                ImGui::Text("Grounded: %s", gameScene_->IsPlayerGrounded() ? "Yes" : "No");
            }
            else if (secondaryInspectorTarget_ == SecondaryInspectorTarget::Enemy && gameScene_) {
                const std::vector<Vector3>& enemyPositions = gameScene_->GetEnemyPositions();
                if (secondaryEnemyIndex_ >= 0 &&
                    secondaryEnemyIndex_ < static_cast<int32_t>(enemyPositions.size())) {
                    Vector3 enemyPosition = enemyPositions[static_cast<size_t>(secondaryEnemyIndex_)];
                    float enemyHealth = gameScene_->GetEnemyHealth(static_cast<size_t>(secondaryEnemyIndex_));
                    ImGui::Text("Enemy %d", secondaryEnemyIndex_ + 1);
                    ImGui::Separator();
                    if (ImGui::SliderFloat(
                        "HP", &enemyHealth, 0.0f, gameScene_->GetEnemyMaxHealth(), "%.0f")) {
                        gameScene_->SetEnemyHealth(static_cast<size_t>(secondaryEnemyIndex_), enemyHealth);
                    }
                    ImGui::Text("World Position");
                    const bool positionChanged =
                        ImGui::SliderFloat("Position X", &enemyPosition.x, -30.0f, 30.0f, "%.2f") |
                        ImGui::SliderFloat("Position Y", &enemyPosition.y, -20.0f, 20.0f, "%.2f") |
                        ImGui::SliderFloat("Position Z", &enemyPosition.z, -5.0f, 5.0f, "%.2f");
                    if (positionChanged) {
                        gameScene_->SetEnemyPosition(static_cast<size_t>(secondaryEnemyIndex_), enemyPosition);
                    }
                }
                else {
                    ImGui::TextDisabled("Enemy is not available");
                }
            }
            ImGui::End();
            if (!isSecondaryInspectorOpen) {
                secondaryInspectorTarget_ = SecondaryInspectorTarget::None;
                secondaryEnemyIndex_ = -1;
            }
        }

        ImGui::Begin("Console", nullptr, panelFlags);
        ImGui::TextColored(ImVec4(0.38f, 0.78f, 0.52f, 1.0f), "Editor ready");
        ImGui::SameLine();
        ImGui::TextDisabled("| %.1f FPS", ImGui::GetIO().Framerate);
        ImGui::Separator();
        if (sceneManager_) {
            ImGui::Text("Current Scene: %s", sceneManager_->GetCurrentSceneName());
            ImGui::Text("Game Time: %.2f sec", sceneManager_->GetElapsedGameTimeSeconds());
            ImGui::TextDisabled("Transition: %s", sceneManager_->GetTransitionCondition());
            ImGui::Separator();
        }
        if (gameScene_) {
            ImGui::Text("Player Ammo: %d / 10", gameScene_->GetRemainingProjectileCount());
            bool isOrientationMarkerVisible =
                gameScene_->IsOrientationMarkerVisible();
            if (ImGui::Checkbox(
                "Show Axis Marker",
                &isOrientationMarkerVisible)) {
                gameScene_->SetOrientationMarkerVisible(
                    isOrientationMarkerVisible);
            }
            ImGui::Separator();
        }
        ImGui::TextColored(ImVec4(0.95f, 0.15f, 0.15f, 1.0f), "+X: Red");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.15f, 0.95f, 0.25f, 1.0f), "+Y: Green");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.20f, 0.45f, 1.00f, 1.0f), "+Z: Blue");
        ImGui::End();
#endif
    }

}
