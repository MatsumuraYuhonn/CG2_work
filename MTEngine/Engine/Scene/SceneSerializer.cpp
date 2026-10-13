#include "SceneSerializer.h"

#include <filesystem>
#include <fstream>
#include <system_error>

#include "MTEngine/Engine/externals/json.hpp"

namespace MTEngine {

	using json = nlohmann::json;

	bool SceneSerializer::Save(const std::string& filePath, const SceneObject& object) {

		json root;

		root["objects"] = json::array();

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

	bool SceneSerializer::Load(const std::string& filePath, SceneObject& object)
	{

		std::ifstream file(filePath);

		if (!file.is_open()) {
			return false;
		}

		try {

			json root;

			file >> root;

			const json& objectData = root.at("objects").at(0);

			const json& transformData =
				objectData.at("transform");

			const json& position =
				transformData.at("position");

			const json& rotation =
				transformData.at("rotation");

			const json& scale =
				transformData.at("scale");



			const uint64_t loadedId =
				objectData.at("id").get<uint64_t>();

			const std::string loadedName =
				objectData.at("name").get<std::string>();

			const std::string loadedModelPath =
				objectData.at("modelPath").get<std::string>();


			Transform loadedTransform;

			loadedTransform.translate = {
				position.at(0).get<float>(),
				position.at(1).get<float>(),
				position.at(2).get<float>()
			};

			loadedTransform.rotate = {
				rotation.at(0).get<float>(),
				rotation.at(1).get<float>(),
				rotation.at(2).get<float>()
			};

			loadedTransform.scale = {
				scale.at(0).get<float>(),
				scale.at(1).get<float>(),
				scale.at(2).get<float>()
			};


			// 全部正常に読めたらSceneObjectへ反映する
			object.id = loadedId;
			object.name = loadedName;
			object.modelPath = loadedModelPath;
			object.transform = loadedTransform;

			return true;
		}

		catch (const json::exception&) {
			// JSONの項目不足や形式不正
			return false;
		}

	}

}
