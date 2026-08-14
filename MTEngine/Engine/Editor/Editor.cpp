#include "Editor.h"
#include "MTEngine/Engine/Debug/DebugCamera.h"
#include "MTEngine/Game/Scene/GameScene.h"

#include <cstdio>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_internal.h"
#endif

namespace MTEngine {

    void Editor::Initialize(
        D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle,
        GameScene* gameScene,
        DebugCamera* debugCamera) {
        sceneTextureHandle_ = sceneTextureHandle;
        gameScene_ = gameScene;
        debugCamera_ = debugCamera;
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
            ImGuiID leftNode = 0;
            ImGuiID bottomNode = 0;
            ImGui::DockBuilderSplitNode(centerNode, ImGuiDir_Right, 0.25f, &rightNode, &centerNode);
            ImGui::DockBuilderSplitNode(centerNode, ImGuiDir_Left, 0.20f, &leftNode, &centerNode);
            ImGui::DockBuilderSplitNode(centerNode, ImGuiDir_Down, 0.30f, &bottomNode, &centerNode);
            ImGui::DockBuilderDockWindow("Hierarchy", leftNode);
            ImGui::DockBuilderDockWindow("Inspector", rightNode);
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
                    }
                }
                ImGui::TreePop();
            }
            if (ImGui::Selectable("Player", isPlayerSelected_)) {
                isPlayerSelected_ = true;
                isDebugCameraSelected_ = false;
                selectedTileIndex_ = -1;
            }
        }
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Scene", nullptr, panelFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
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
            if (ImGui::SliderFloat("Acceleration", &acceleration, 0.0f, 50.0f, "%.1f")) {
                gameScene_->SetPlayerMoveAcceleration(acceleration);
            }
            if (ImGui::SliderFloat("Velocity Damping", &damping, 0.0f, 20.0f, "%.1f")) {
                gameScene_->SetPlayerVelocityDamping(damping);
            }
            if (ImGui::SliderFloat("Max Move Speed", &maxMoveSpeed, 0.0f, 20.0f, "%.1f")) {
                gameScene_->SetPlayerMaxMoveSpeed(maxMoveSpeed);
            }
            ImGui::Text("Horizontal Velocity: %.2f", gameScene_->GetPlayerHorizontalVelocity());
            ImGui::Text("Vertical Velocity: %.2f", gameScene_->GetPlayerVerticalVelocity());
            ImGui::Text("Grounded: %s", gameScene_->IsPlayerGrounded() ? "Yes" : "No");
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

        ImGui::Begin("Console", nullptr, panelFlags);
        ImGui::TextColored(ImVec4(0.38f, 0.78f, 0.52f, 1.0f), "Editor ready");
        ImGui::SameLine();
        ImGui::TextDisabled("| %.1f FPS", ImGui::GetIO().Framerate);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.95f, 0.15f, 0.15f, 1.0f), "+X: Red");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.15f, 0.95f, 0.25f, 1.0f), "+Y: Green");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.20f, 0.45f, 1.00f, 1.0f), "+Z: Blue");
        ImGui::End();
#endif
    }

}
