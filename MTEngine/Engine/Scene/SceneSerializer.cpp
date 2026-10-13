#include "SceneSerializer.h"

#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>

#include "MTEngine/Engine/externals/json.hpp"

namespace MTEngine {

    using json = nlohmann::json;

    bool SceneSerializer::Save(const std::string& filePath, const Scene& scene) {

        json root;

        root["objects"] = json::array();

        for (const SceneObject& object : scene.GetObjects()) {
            const Transform& transform = object.transform;

            root["objects"].push_back({
                {"id", object.id},
                {"name", object.name},
                {"modelPath", object.modelPath },

                {
                    "transform",
                    {

                        {
                            "position",
                            {
                                transform.translate.x,
                                transform.translate.y,
                                transform.translate.z
                            }
                        },

                        {
                            "rotation",
                            {
                                transform.rotate.x,
                                transform.rotate.y,
                                transform.rotate.z

                            }

                        },

                        {
                            "scale",
                            {
                                transform.scale.x,
                                transform.scale.y,
                                transform.scale.z

                            }


                        }
                    }
                }

                });
        }

        const std::filesystem::path path(filePath);

        std::error_code error;
        std::filesystem::create_directories((path.parent_path()), error);

        if (error) {

            return false;

        }

        std::ofstream file(path);

        if (!file.is_open()) {

            return false;

        }

        file << root.dump(4);

        return true;

    }

    bool SceneSerializer::Load(
        const std::string& filePath,
        Scene& scene) {

        std::ifstream file(filePath);

        if (!file.is_open()) {
            return false;
        }

        try {
            json root;
            file >> root;

            const json& objectsData = root.at("objects");

            if (!objectsData.is_array()) {
                return false;
            }

            // 読み込み途中で失敗しても、元のsceneを壊さないようにする
            Scene loadedScene;

            for (const json& objectData : objectsData) {
                const json& transformData =
                    objectData.at("transform");

                const json& position =
                    transformData.at("position");

                const json& rotation =
                    transformData.at("rotation");

                const json& scale =
                    transformData.at("scale");

                SceneObject object;

                object.id =
                    objectData.at("id").get<uint64_t>();

                object.name =
                    objectData.at("name").get<std::string>();

                object.modelPath =
                    objectData.at("modelPath").get<std::string>();

                object.transform.translate = {
                    position.at(0).get<float>(),
                    position.at(1).get<float>(),
                    position.at(2).get<float>()
                };

                object.transform.rotate = {
                    rotation.at(0).get<float>(),
                    rotation.at(1).get<float>(),
                    rotation.at(2).get<float>()
                };

                object.transform.scale = {
                    scale.at(0).get<float>(),
                    scale.at(1).get<float>(),
                    scale.at(2).get<float>()
                };

                loadedScene.AddObject(std::move(object));
            }

            // 全オブジェクトを正常に読めた場合のみ反映する
            scene = std::move(loadedScene);

            return true;
        }
        catch (const json::exception&) {
            // JSONの項目不足、型違い、配列不足など
            return false;
        }
    }

}