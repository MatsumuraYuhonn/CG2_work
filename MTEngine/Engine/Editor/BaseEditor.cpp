#include "BaseEditor.h"

#include "MTEngine/Engine/Debug/DebugCamera.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_internal.h"
#endif

namespace MTEngine {

    void BaseEditor::Initialize(
        D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle,
        DebugCamera* debugCamera) {
        sceneTextureHandle_ = sceneTextureHandle;
        debugCamera_ = debugCamera;
    }

    void BaseEditor::Update() {
#ifdef USE_IMGUI
        if (!isLayoutInitialized_) {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            const ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
            ImGui::DockBuilderRemoveNode(dockspaceId);
            ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

            ImGuiID centerNode = dockspaceId;
            ImGuiID leftNode = 0;
            ImGuiID rightNode = 0;
            ImGuiID bottomNode = 0;
            ImGui::DockBuilderSplitNode(
                centerNode, ImGuiDir_Left, 0.20f, &leftNode, &centerNode);
            ImGui::DockBuilderSplitNode(
                centerNode, ImGuiDir_Right, 0.25f, &rightNode, &centerNode);
            ImGui::DockBuilderSplitNode(
                centerNode, ImGuiDir_Down, 0.25f, &bottomNode, &centerNode);

            ImGui::DockBuilderDockWindow("Hierarchy", leftNode);
            ImGui::DockBuilderDockWindow("Inspector", rightNode);
            ImGui::DockBuilderDockWindow("Console", bottomNode);
            ImGui::DockBuilderDockWindow("Scene", centerNode);
            ImGui::DockBuilderFinish(dockspaceId);
            isLayoutInitialized_ = true;
        }

        constexpr ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoCollapse;

        ImGui::Begin("Hierarchy", nullptr, panelFlags);
        ImGui::TextDisabled("Engine Objects");
        ImGui::Separator();
        if (ImGui::TreeNodeEx(
            "MTEngine",
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (ImGui::Selectable("Debug Camera", isDebugCameraSelected_)) {
                isDebugCameraSelected_ = true;
            }
            ImGui::TreePop();
        }
        ImGui::Spacing();
        ImGui::TextDisabled("Game objects can be added here later.");
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin(
            "Scene",
            nullptr,
            panelFlags |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse);
        const ImVec2 sceneSize = ImGui::GetContentRegionAvail();
        if (sceneSize.x > 0.0f && sceneSize.y > 0.0f) {
            ImGui::Image(
                ImTextureRef(static_cast<ImTextureID>(sceneTextureHandle_.ptr)),
                sceneSize);
        }
        isSceneViewFocused_ =
            ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
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
            ImGui::Text("Target");
            if (ImGui::DragFloat3("##Target", &target.x, 0.1f)) {
                debugCamera_->SetTarget(target);
            }
            ImGui::Separator();
            if (ImGui::DragFloat(
                "Distance", &distance, 0.1f, 0.1f, 1000.0f, "%.2f")) {
                debugCamera_->SetDistance(distance);
            }
            if (ImGui::SliderAngle("Yaw", &yaw, -360.0f, 360.0f)) {
                debugCamera_->SetYaw(yaw);
            }
            if (ImGui::SliderAngle("Pitch", &pitch, -89.0f, 89.0f)) {
                debugCamera_->SetPitch(pitch);
            }
        } else {
            ImGui::TextDisabled("No object selected");
        }
        ImGui::End();

        ImGui::Begin("Console", nullptr, panelFlags);
        ImGui::TextColored(
            ImVec4(0.38f, 0.78f, 0.52f, 1.0f),
            "MTEngine editor ready");
        ImGui::SameLine();
        ImGui::TextDisabled("| %.1f FPS", ImGui::GetIO().Framerate);
        ImGui::Separator();
        ImGui::TextWrapped(
            "Add game-specific panels from GameApplication::OnUpdate().");
        ImGui::End();
#else
        isSceneViewFocused_ = true;
#endif
    }

}
