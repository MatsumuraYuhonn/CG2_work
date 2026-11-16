#pragma once

#include <d3d12.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace MTEngine {

    class DebugCamera;
    class Scene;

    // ゲーム固有クラスに依存しない、MTEngine 共通のエディターUI。
    class BaseEditor {
    public:
        void Initialize(
            D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle,
            DebugCamera* debugCamera, Scene* scene);
        void Update();

        // ゲーム固有オブジェクトをHierarchyとInspectorへ登録する。
        void RegisterGameObject(
            const std::string& name,
            std::function<void()> drawInspector);

        bool IsSceneViewFocused() const {
            return isSceneViewFocused_ && !isGizmoUsing_;
        }

    private:
        enum class GizmoOperation {
            Translate,
            Rotate,
            Scale
        };

        enum class GizmoSpace {
            Local,
            World
        };

        struct GameObjectEntry {
            std::string name;
            std::function<void()> drawInspector;
        };

        struct ModelAssetEntry {
            std::string displayName;
            std::string modelPath;
        };

        void SetSelectedObjectName(const std::string& name);
        void RefreshModelAssets();
        void SelectModelAsset(size_t index);

        D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureHandle_{};
        
        DebugCamera* debugCamera_ = nullptr;
        Scene* scene_ = nullptr;

        uint64_t selectedObjectId_ = 0;
        size_t selectedGameObjectIndex_ = 0;

        bool isSceneViewFocused_ = false;
        bool isGizmoUsing_ = false;
        bool isDebugCameraSelected_ = true;
        bool isLayoutInitialized_ = false;

        GizmoOperation gizmoOperation_ = GizmoOperation::Translate;
        GizmoSpace gizmoSpace_ = GizmoSpace::World;

        char newObjectName_[128] = "New Object";
        char newModelPath_[512] = "";
        char selectedObjectName_[128] = "";

        std::vector<GameObjectEntry> gameObjects_;
        std::vector<ModelAssetEntry> modelAssets_;
        int selectedModelAssetIndex_ = -1;

    };

}
