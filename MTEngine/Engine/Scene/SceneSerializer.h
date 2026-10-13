#pragma once
#include<string>

#include "MTEngine/Engine/Scene/Scene.h"

namespace MTEngine {

	//  SceneObjectをJSONへ保存・読込するクラス
	class SceneSerializer
	{

    public:
        // objectの内容をJSONファイルへ保存する
        static bool Save(
            const std::string& filePath,
            const Scene& scene);

        // JSONファイルからobjectへ読み込む
        static bool Load(
            const std::string& filePath,
            Scene& scene);

	};

}