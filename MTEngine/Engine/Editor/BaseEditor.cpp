#include "BaseEditor.h"

#include "MTEngine/Engine/Debug/DebugCamera.h"
#include "MTEngine/Engine/Scene/Scene.h"

#include <cstdio>
#include <utility>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_internal.h"
#endif

namespace MTEngine {

    void BaseEditor::Initialize(
        D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle,
        DebugCamera* debugCamera, Scene* scene) {
        sceneTextureHandle_ = sceneTextureHandle;
        debugCamera_ = debugCamera;
		scene_ = scene;
    }

    void BaseEditor::RegisterGameObject(
        const std::string& name,
        std::function<void()> drawInspector) {
        gameObjects_.push_back({ name, std::move(drawInspector) });
    }

    void BaseEditor::SetSelectedObjectName(const std::string& name) {
        std::snprintf(
            selectedObjectName_,
            sizeof(selectedObjectName_),
            "%s",
            name.c_str());
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
            if (ImGui::Selectable(
                "Debug Camera",
                isDebugCameraSelected_)) {

                isDebugCameraSelected_ = true;
                selectedObjectId_ = 0;
                selectedGameObjectIndex_ = 0;
                SetSelectedObjectName("");
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();

        ImGui::TextDisabled("Scene Objects");
        ImGui::Separator();

        if (scene_ && ImGui::Button("+ Add Object")) {
            ImGui::OpenPopup("Add Scene Object");
        }

        if (ImGui::BeginPopupModal(
            "Add Scene Object",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {

            ImGui::InputText(
                "Name",
                newObjectName_,
                sizeof(newObjectName_));

            ImGui::InputText(
                "Model Path",
                newModelPath_,
                sizeof(newModelPath_));

            const bool canCreate =
                newObjectName_[0] != '\0' &&
                newModelPath_[0] != '\0';

            if (!canCreate) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("Create")) {
                selectedObjectId_ = scene_->CreateObject(
                    newObjectName_,
                    newModelPath_);

                isDebugCameraSelected_ = false;
                selectedGameObjectIndex_ = 0;
                SetSelectedObjectName(newObjectName_);

                ImGui::CloseCurrentPopup();
            }

            if (!canCreate) {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (scene_) {
            const std::vector<SceneObject>& objects =
                scene_->GetObjects();

            if (objects.empty()) {
                ImGui::TextDisabled("Empty Scene");
            }

            for (const SceneObject& object : objects) {
                const bool isSelected =
                    !isDebugCameraSelected_ &&
                    selectedObjectId_ == object.id;

                // 同じ名前のオブジェクトがあっても区別できるラベル
                const std::string label =
                    object.name +
                    "##SceneObject" +
                    std::to_string(object.id);

                if (ImGui::Selectable(
                    label.c_str(),
                    isSelected)) {

                    selectedObjectId_ = object.id;
                    isDebugCameraSelected_ = false;
                    selectedGameObjectIndex_ = 0;
                    SetSelectedObjectName(object.name);
                }
            }

            if (scene_ && selectedObjectId_ != 0) {
                ImGui::Spacing();

                if (ImGui::Button("Duplicate Selected")) {
                    const uint64_t duplicatedObjectId =
                        scene_->DuplicateObject(selectedObjectId_);

                    if (duplicatedObjectId != 0) {
                        selectedObjectId_ = duplicatedObjectId;
                        isDebugCameraSelected_ = false;
                        selectedGameObjectIndex_ = 0;

                        const SceneObject* duplicatedObject =
                            scene_->FindObject(duplicatedObjectId);
                        if (duplicatedObject) {
                            SetSelectedObjectName(duplicatedObject->name);
                        }
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Delete Selected")) {
                    scene_->RemoveObject(selectedObjectId_);

                    selectedObjectId_ = 0;
                    isDebugCameraSelected_ = true;
                    selectedGameObjectIndex_ = 0;
                    SetSelectedObjectName("");
                }
            }
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Game Objects");
        ImGui::Separator();

        if (gameObjects_.empty()) {
            ImGui::TextDisabled("No registered game objects");
        }

        for (size_t index = 0; index < gameObjects_.size(); ++index) {
            const size_t selectionIndex = index + 1;
            const std::string label =
                gameObjects_[index].name +
                "##GameObject" +
                std::to_string(selectionIndex);

            if (ImGui::Selectable(
                label.c_str(),
                selectedGameObjectIndex_ == selectionIndex)) {
                selectedGameObjectIndex_ = selectionIndex;
                selectedObjectId_ = 0;
                isDebugCameraSelected_ = false;
                SetSelectedObjectName(gameObjects_[index].name);
            }
        }

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

            if (ImGui::Button("Reset Camera")) {
                debugCamera_->Reset();
            }

        } else if (scene_ && selectedObjectId_ != 0) {

            SceneObject* selectedObject =
                scene_->FindObject(selectedObjectId_);

            if (selectedObject) {
           
                if (ImGui::InputText(
                    "Name",
                    selectedObjectName_,
                    sizeof(selectedObjectName_)) &&
                    selectedObjectName_[0] != '\0') {
                    selectedObject->name = selectedObjectName_;
                }

                ImGui::TextDisabled(
                    "ID: %llu",
                    static_cast<unsigned long long>(selectedObject->id));

                ImGui::Separator();

                Transform& transform =
                    selectedObject->transform;

                ImGui::Text("Transform");

                ImGui::DragFloat3(
                    "Position",
                    &transform.translate.x,
                    0.1f);

                constexpr float kRadiansToDegrees =
                    180.0f / 3.14159265f;

                constexpr float kDegreesToRadians =
                    3.14159265f / 180.0f;

                float rotationDegrees[3] = {
                    transform.rotate.x * kRadiansToDegrees,
                    transform.rotate.y * kRadiansToDegrees,
                    transform.rotate.z * kRadiansToDegrees
                };

                if (ImGui::DragFloat3(
                    "Rotation",
                    rotationDegrees,
                    0.5f)) {

                    transform.rotate.x =
                        rotationDegrees[0] * kDegreesToRadians;

                    transform.rotate.y =
                        rotationDegrees[1] * kDegreesToRadians;

                    transform.rotate.z =
                        rotationDegrees[2] * kDegreesToRadians;
                }

                ImGui::DragFloat3(
                    "Scale",
                    &transform.scale.x,
                    0.01f);

                if (ImGui::Button("Reset Transform")) {
                    transform = Transform{};
                }

                ImGui::Separator();

                ImGui::TextDisabled("Model");
                ImGui::TextWrapped(
                    "%s",
                    selectedObject->modelPath.c_str());

            } else {
                selectedObjectId_ = 0;
                ImGui::TextDisabled("No object selected");
            }

        } else if (
            selectedGameObjectIndex_ > 0 &&
            selectedGameObjectIndex_ <= gameObjects_.size()) {
            GameObjectEntry& selectedObject =
                gameObjects_[selectedGameObjectIndex_ - 1];

            if (ImGui::InputText(
                "Name",
                selectedObjectName_,
                sizeof(selectedObjectName_)) &&
                selectedObjectName_[0] != '\0') {
                selectedObject.name = selectedObjectName_;
            }
            ImGui::Separator();
            if (selectedObject.drawInspector) {
                selectedObject.drawInspector();
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
