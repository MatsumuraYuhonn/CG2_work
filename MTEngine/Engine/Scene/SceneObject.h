#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "MTEngine/Engine/Graphics/ConstantBuffer.h"
#include "MTEngine/Engine/Graphics/Model.h"
#include "MTEngine/Engine/Math/Transform.h"
							
namespace MTEngine {

	enum class ModelLoadState {
		Unloaded,
		Loaded,
		Failed
	};

	// シーンに配置された一個のモデル
	struct SceneObject {
		uint64_t id = 0; // モデルを識別するID
		std::string name; // モデルの名前
		std::string modelPath; // モデルのパス
		Transform transform; // オブジェクトの変換情報（位置、回転、スケール）

		std::shared_ptr<Model> model; // モデルの共有ポインタ
		ModelLoadState modelLoadState = ModelLoadState::Unloaded;

		// 変換行列を格納する定数バッファ
		std::unique_ptr <TransformationMatrixConstantBuffer> transformBuffer;
	};


}
