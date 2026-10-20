#include "Scene.h"

#include <algorithm>
#include <utility>

namespace MTEngine {

    uint64_t Scene::CreateObject(
        const std::string& name,
        const std::string& modelPath) {

        SceneObject object;
        object.name = name;
        object.modelPath = modelPath;

        return AddObject(std::move(object));
    }

    uint64_t Scene::AddObject(SceneObject object) {
        // ID未設定、または重複している場合は新しいIDを割り当てる
        if (object.id == 0 || FindObject(object.id) != nullptr) {
            object.id = GenerateId();
        }
        else {
            // JSONから復元したIDより次のIDを進める
            nextId_ = (std::max)(nextId_, object.id + 1);
        }

        const uint64_t id = object.id;
        objects_.push_back(std::move(object));
        return id;
    }

    bool Scene::RemoveObject(uint64_t id) {
        const auto iterator = std::remove_if(
            objects_.begin(),
            objects_.end(),
            [id](const SceneObject& object) {
                return object.id == id;
            });

        if (iterator == objects_.end()) {
            return false;
        }

        objects_.erase(iterator, objects_.end());
        return true;
    }

    uint64_t Scene::DuplicateObject(uint64_t id) {
        const SceneObject* source = FindObject(id);
        if (!source) {
            return 0;
        }

        // SceneObjectは描画用リソースを持つため、オブジェクト全体はコピーしない。
        // シーンとして保存する情報だけを引き継ぎ、描画用リソースは
        // SceneRendererに新しく作成してもらう。
        SceneObject duplicate;
        duplicate.name = source->name + " Copy";
        duplicate.modelPath = source->modelPath;
        duplicate.transform = source->transform;

        return AddObject(std::move(duplicate));
    }

    SceneObject* Scene::FindObject(uint64_t id) {
        const auto iterator = std::find_if(
            objects_.begin(),
            objects_.end(),
            [id](const SceneObject& object) {
                return object.id == id;
            });

        if (iterator == objects_.end()) {
            return nullptr;
        }

        return &(*iterator);
    }

    const SceneObject* Scene::FindObject(uint64_t id) const {
        const auto iterator = std::find_if(
            objects_.begin(),
            objects_.end(),
            [id](const SceneObject& object) {
                return object.id == id;
            });

        if (iterator == objects_.end()) {
            return nullptr;
        }

        return &(*iterator);
    }

    std::vector<SceneObject>& Scene::GetObjects() {
        return objects_;
    }

    const std::vector<SceneObject>& Scene::GetObjects() const {
        return objects_;
    }

    void Scene::Clear() {
        objects_.clear();
        nextId_ = 1;
    }

    uint64_t Scene::GenerateId() {
        while (FindObject(nextId_) != nullptr) {
            ++nextId_;
        }

        return nextId_++;
    }

}
