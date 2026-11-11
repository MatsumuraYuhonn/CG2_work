#include "BaseEditor.h"

#include "MTEngine/Engine/Debug/DebugCamera.h"
#include "MTEngine/Engine/Scene/Scene.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <system_error>
#include <utility>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_internal.h"
#include "MTEngine/Engine/externals/ImGuizmo/ImGuizmo.h"
#endif

namespace MTEngine {

    namespace {

        constexpr const char* kModelAssetDirectory =
            "MTEngine/Assets/Resources";

    }

    void BaseEditor::Initialize(
        D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle,
        DebugCamera* debugCamera, Scene* scene) {
        sceneTextureHandle_ = sceneTextureHandle;
        debugCamera_ = debugCamera;
		scene_ = scene;
        RefreshModelAssets();
    }

    void BaseEditor::RegisterGameObject(
        const std::string& name,
        std::function<void()> drawInspector,
        std::function<void()> drawSceneVisualization) {
        gameObjects_.push_back({
            name,
            std::move(drawInspector),
            std::move(drawSceneVisualization)
        });
    }

    void BaseEditor::SetSelectedObjectName(const std::string& name) {
        std::snprintf(
            selectedObjectName_,
            sizeof(selectedObjectName_),
            "%s",
            name.c_str());
    }

    void BaseEditor::RefreshModelAssets() {
        modelAssets_.clear();
        selectedModelAssetIndex_ = -1;

        const std::filesystem::path assetDirectory(kModelAssetDirectory);
        std::error_code error;
        std::filesystem::recursive_directory_iterator iterator(
            assetDirectory,
            std::filesystem::directory_options::skip_permission_denied,
            error);
        const std::filesystem::recursive_directory_iterator end;

        while (!error && iterator != end) {
            const std::filesystem::directory_entry& entry = *iterator;
            if (entry.is_regular_file(error)) {
                std::string extension = entry.path().extension().string();
                std::transform(
                    extension.begin(),
                    extension.end(),
                    extension.begin(),
                    [](unsigned char character) {
                        return static_cast<char>(std::tolower(character));
                    });

                if (extension == ".obj") {
                    ModelAssetEntry modelAsset;
                    modelAsset.displayName = entry.path()
                        .lexically_relative(assetDirectory)
                        .generic_string();
                    modelAsset.modelPath = entry.path().generic_string();
                    modelAssets_.push_back(std::move(modelAsset));
                }
            }

            iterator.increment(error);
        }

        std::sort(
            modelAssets_.begin(),
            modelAssets_.end(),
            [](const ModelAssetEntry& left, const ModelAssetEntry& right) {
                return left.displayName < right.displayName;
            });

        for (size_t index = 0; index < modelAssets_.size(); ++index) {
            if (modelAssets_[index].modelPath == newModelPath_) {
                selectedModelAssetIndex_ = static_cast<int>(index);
                break;
            }
        }
    }

    void BaseEditor::SelectModelAsset(size_t index) {
        if (index >= modelAssets_.size()) {
            return;
        }

        selectedModelAssetIndex_ = static_cast<int>(index);
        std::snprintf(
            newModelPath_,
            sizeof(newModelPath_),
            "%s",
            modelAssets_[index].modelPath.c_str());
    }

    void BaseEditor::Update() {
#ifdef USE_IMGUI
        ImGuizmo::BeginFrame();
        isGizmoUsing_ = false;

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
            RefreshModelAssets();
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

            const char* modelPreview = "Select OBJ...";
            if (selectedModelAssetIndex_ >= 0 &&
                selectedModelAssetIndex_ <
                    static_cast<int>(modelAssets_.size())) {

                modelPreview = modelAssets_[selectedModelAssetIndex_]
                    .displayName.c_str();
            }

            if (ImGui::BeginCombo("Model", modelPreview)) {
                for (size_t index = 0; index < modelAssets_.size(); ++index) {
                    const bool isSelected =
                        selectedModelAssetIndex_ == static_cast<int>(index);

                    if (ImGui::Selectable(
                        modelAssets_[index].displayName.c_str(),
                        isSelected)) {

                        SelectModelAsset(index);
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            if (modelAssets_.empty()) {
                ImGui::TextDisabled(
                    "No OBJ files in %s",
                    kModelAssetDirectory);
            }

            if (ImGui::Button("Refresh Models")) {
                RefreshModelAssets();
            }

            ImGui::InputText(
                "Model Path (Manual)",
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

        const bool isSceneWindowFocused =
            ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        if (isSceneWindowFocused &&
            !ImGui::GetIO().WantTextInput &&
            !ImGuizmo::IsUsing()) {
            if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
                gizmoOperation_ = GizmoOperation::Translate;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
                gizmoOperation_ = GizmoOperation::Rotate;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
                gizmoOperation_ = GizmoOperation::Scale;
            }
        }

        int operationIndex = static_cast<int>(gizmoOperation_);
        ImGui::RadioButton("Move (W)", &operationIndex, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Rotate (E)", &operationIndex, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Scale (R)", &operationIndex, 2);
        gizmoOperation_ = static_cast<GizmoOperation>(operationIndex);

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        const bool isRotationOperation =
            gizmoOperation_ == GizmoOperation::Rotate;

        int spaceIndex = static_cast<int>(gizmoSpace_);
        if (isRotationOperation) {
            spaceIndex = static_cast<int>(GizmoSpace::Local);
            ImGui::BeginDisabled();
        }

        ImGui::RadioButton("Local", &spaceIndex, 0);
        ImGui::SameLine();
        ImGui::RadioButton("World", &spaceIndex, 1);

        if (isRotationOperation) {
            ImGui::EndDisabled();
        } else {
            gizmoSpace_ = static_cast<GizmoSpace>(spaceIndex);
        }

        ImGui::Separator();

        const ImVec2 scenePosition = ImGui::GetCursorScreenPos();
        const ImVec2 sceneSize = ImGui::GetContentRegionAvail();
        if (sceneSize.x > 0.0f && sceneSize.y > 0.0f) {
            ImGui::Image(
                ImTextureRef(static_cast<ImTextureID>(sceneTextureHandle_.ptr)),
                sceneSize);

            if (debugCamera_) {
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
                ImGuizmo::SetRect(
                    scenePosition.x,
                    scenePosition.y,
                    sceneSize.x,
                    sceneSize.y);

                if (selectedGameObjectIndex_ > 0) {
                    const size_t gameObjectIndex =
                        selectedGameObjectIndex_ - 1;
                    if (gameObjectIndex < gameObjects_.size()) {
                        const std::function<void()>& drawVisualization =
                            gameObjects_[gameObjectIndex]
                                .drawSceneVisualization;
                        if (drawVisualization) {
                            drawVisualization();
                        }
                    }
                }
            }

            SceneObject* selectedObject = nullptr;
            if (scene_ && selectedObjectId_ != 0) {
                selectedObject = scene_->FindObject(selectedObjectId_);
            }

            if (selectedObject && debugCamera_) {
                Transform& transform = selectedObject->transform;
                Matrix4x4 world = MakeAffineMatrix(
                    transform.scale,
                    transform.rotate,
                    transform.translate);

                ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
                if (gizmoOperation_ == GizmoOperation::Rotate) {
                    operation =
                        ImGuizmo::ROTATE_X |
                        ImGuizmo::ROTATE_Y |
                        ImGuizmo::ROTATE_Z;
                }
                if (gizmoOperation_ == GizmoOperation::Scale) {
                    operation = ImGuizmo::SCALE;
                }

                ImGuizmo::MODE mode = ImGuizmo::LOCAL;
                if (gizmoOperation_ != GizmoOperation::Rotate &&
                    gizmoSpace_ == GizmoSpace::World) {
                    mode = ImGuizmo::WORLD;
                }

                const uint64_t highId = selectedObjectId_ >> 32;
                const uint64_t lowId = selectedObjectId_ & 0xffffffffULL;
                const int gizmoId = static_cast<int>(highId ^ lowId);

                Matrix4x4 deltaMatrix = MakeIdentityMatrix();

                ImGuizmo::PushID(gizmoId);
                const bool wasManipulated = ImGuizmo::Manipulate(
                    &debugCamera_->GetViewMatrix().m[0][0],
                    &debugCamera_->GetProjectionMatrix().m[0][0],
                    operation,
                    mode,
                    &world.m[0][0],
                    &deltaMatrix.m[0][0]);
                ImGuizmo::PopID();

                isGizmoUsing_ = ImGuizmo::IsUsing();

                if (wasManipulated) {
                    if (gizmoOperation_ == GizmoOperation::Translate) {
                        transform.translate = {
                            world.m[3][0],
                            world.m[3][1],
                            world.m[3][2]
                        };
                    }

                    if (gizmoOperation_ == GizmoOperation::Rotate) {
                        const int rotationAxis =
                            ImGuizmo::GetActiveRotationAxis();
                        const float rotationDelta =
                            ImGuizmo::GetRotationDelta();

                        if (rotationAxis == 0) {
                            transform.rotate.x += rotationDelta;
                        }
                        if (rotationAxis == 1) {
                            transform.rotate.y += rotationDelta;
                        }
                        if (rotationAxis == 2) {
                            transform.rotate.z += rotationDelta;
                        }
                    }

                    if (gizmoOperation_ == GizmoOperation::Scale) {
                        float translation[3]{};
                        float rotationDegrees[3]{};
                        float scale[3]{};
                        ImGuizmo::DecomposeMatrixToComponents(
                            &world.m[0][0],
                            translation,
                            rotationDegrees,
                            scale);

                        transform.scale = {
                            scale[0],
                            scale[1],
                            scale[2]
                        };
                    }
                }
            }
        }
        isSceneViewFocused_ = isSceneWindowFocused;
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
