#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "MTEngine/Engine/Scene/SceneObject.h"

namespace MTEngine {

    class Scene {
    public:
        // 新しいSceneObjectを作成して、そのIDを返す
        uint64_t CreateObject(
            const std::string& name,
            const std::string& modelPath);

        // 既存のSceneObjectをSceneへ移動する
        uint64_t AddObject(SceneObject object);

        // IDを指定して削除する
        bool RemoveObject(uint64_t id);

        // IDからオブジェクトを検索する
        SceneObject* FindObject(uint64_t id);
        const SceneObject* FindObject(uint64_t id) const;

        // Hierarchyや保存処理で一覧を参照する
        const std::vector<SceneObject>& GetObjects() const;

        // シーン内のオブジェクトをすべて削除する
        void Clear();

    private:
        uint64_t GenerateId();

        std::vector<SceneObject> objects_;
        uint64_t nextId_ = 1;
    };

}

