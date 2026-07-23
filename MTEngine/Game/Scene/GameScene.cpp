#include "GameScene.h"
#include <cassert>
#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif


namespace MTEngine {

	Matrix4x4 GameScene::MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
		Matrix4x4 result = { 0 };
		result.m[0][0] = 2.0f / (right - left);
		result.m[1][1] = 2.0f / (top - bottom);
		result.m[2][2] = 1.0f / (farClip - nearClip);
		result.m[3][0] = -(right + left) / (right - left);
		result.m[3][1] = -(top + bottom) / (top - bottom);
		result.m[3][2] = -nearClip / (farClip - nearClip);
		result.m[3][3] = 1.0f;
		return result;
	}

	void GameScene::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DescriptorHeapManager* srvHeapManager, IXAudio2* xAudio2) {

		// 定数バッファの初期化（1回だけ）
		materialResourceSprite_.Initialize(device);
		directionalLightResource_.Initialize(device);
		wvpResource_.Initialize(device);
		sphereWvpResource_.Initialize(device);
		multiMeshWvpResource_.Initialize(device);

		textureManager_.Initialize(device, srvHeapManager);

		//　axis.objモデルの読み込み
		modelData_ = Model::LoadObjFile("MTEngine/Game/Resources", "axis.obj");
		model_ = std::make_unique<Model>();
		model_->Initialize(device, modelData_);

		//複数メッシュのモデル読み込み
		multiMeshModelData_ = Model::LoadObjFile("MTEngine/Game/Resources", "multiMesh.obj");
		multiMeshModel_ = std::make_unique<Model>();
		multiMeshModel_->Initialize(device, multiMeshModelData_);
		textureManager_.Load(multiMeshModelData_.meshes[0].material.textureFilePath, commandList);

		// GPUハンドル取得
		multiMeshTextureSrvHandleGPU_ = textureManager_.GetGPUDescriptorHandle(
			multiMeshModelData_.meshes[0].material.textureFilePath);


		// 球体モデルの生成
		sphereModelData_.meshes.emplace_back();
		sphereModelData_.meshes.back().vertices = MakeSphere(16);
		sphereModelData_.meshes.back().material.textureFilePath = "MTEngine/Game/Resources/uvChecker.png";
		sphereModel_ = std::make_unique<Model>();
		sphereModel_->Initialize(device, sphereModelData_);

		// テクスチャマネージャとロード
		textureManager_.Load("MTEngine/Game/Resources/uvChecker.png", commandList);
		textureManager_.Load(modelData_.meshes[0].material.textureFilePath, commandList);

		textureSrvHandleGPU_ = textureManager_.GetGPUDescriptorHandle(modelData_.meshes[0].material.textureFilePath);
		sphereTextureSrvHandleGPU_ = textureManager_.GetGPUDescriptorHandle("MTEngine/Game/Resources/uvChecker.png");

		// スプライト初期化（スプライトもuvCheckerを使う）
		sprite_ = std::make_unique<Sprite>();
		sprite_->Initialize(device, 640, 360, sphereTextureSrvHandleGPU_);

		// オーディオ読み込みと再生
		soundData1_ = SoundLoadWave("MTEngine/Game/Resources/Alarm01.wav");

		// カメラ初期化
		debugCamera_.Initialize();

		// 初期パラメータ設定
		materialResourceSprite_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		materialResourceSprite_->enabledLighting = 1;
		materialResourceSprite_->uvTransform = MakeIdentityMatrix();

		directionalLightResource_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		directionalLightResource_->direction = Vector3(0.0f, -1.0f, 1.0f);
		directionalLightResource_->intensity = 1.0f;
	}

	void GameScene::Update(int clientWidth, int clientHeight, const Input* input, IXAudio2* xAudio2) {

		if (input->TriggerKey(DIK_1)) {
			isDebugCameraActive_ = !isDebugCameraActive_;
		}

		Matrix4x4 viewProjectionMatrix;

		if (isDebugCameraActive_) {
			debugCamera_.Update(input);
			viewProjectionMatrix = Multiply(debugCamera_.GetViewMatrix(), debugCamera_.GetProjectionMatrix());
		}
		else {
			Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform_.scale, cameraTransform_.rotate, cameraTransform_.translate);
			Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(clientWidth) / float(clientHeight), 0.1f, 100.0f);
			viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);
		}

		// axis.objモデルのワールド行列
		Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
		wvpResource_->WVP = Multiply(worldMatrix, viewProjectionMatrix);
		wvpResource_->World = worldMatrix;

		// 球体のワールド行列
		Matrix4x4 sphereWorldMatrix = MakeAffineMatrix(sphereTransform_.scale, sphereTransform_.rotate, sphereTransform_.translate);
		sphereWvpResource_->WVP = Multiply(sphereWorldMatrix, viewProjectionMatrix);
		sphereWvpResource_->World = sphereWorldMatrix;

		Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(clientWidth), float(clientHeight), 0.0f, 100.0f);
		sprite_->Update(projectionMatrixSprite);

		Matrix4x4 multiMeshWorldMatrix = MakeAffineMatrix(multiMeshTransform_.scale, multiMeshTransform_.rotate, multiMeshTransform_.translate);
		multiMeshWvpResource_->WVP = Multiply(multiMeshWorldMatrix, viewProjectionMatrix);
		multiMeshWvpResource_->World = multiMeshWorldMatrix;


#ifdef USE_IMGUI
		ImGui::Begin("Debug Settings");

		if (ImGui::CollapsingHeader("Scene Settings")) {
			const char* modeNames[] = { "Default", "MultiMesh", "Sound" };
			int currentMode = static_cast<int>(currentDrawMode_);

			if (ImGui::Combo("Draw Mode", &currentMode, modeNames, IM_ARRAYSIZE(modeNames))) {
				currentDrawMode_ = static_cast<DrawMode>(currentMode);
			}
		}

		switch (currentDrawMode_) {
		case DrawMode::Default:

			// ライト設定
			if (ImGui::CollapsingHeader("Light Settings")) {

				const char* lightingModes[] = { "None", "Lambert", "Half-Lambert" };
				int currentMode = static_cast<int>(materialResourceSprite_->lightingMode);

				if (ImGui::Combo("Lighting Mode", &currentMode, lightingModes, IM_ARRAYSIZE(lightingModes))) {
					materialResourceSprite_->lightingMode = static_cast<float>(currentMode);
				}

				ImGui::Separator();

				ImGui::ColorEdit4("Light Color", &directionalLightResource_->color.x);

				if (ImGui::DragFloat3("Light Direction", &directionalLightResource_->direction.x, 0.01f, -1.0f, 1.0f)) {
					float length = std::sqrt(directionalLightResource_->direction.x * directionalLightResource_->direction.x +
						directionalLightResource_->direction.y * directionalLightResource_->direction.y +
						directionalLightResource_->direction.z * directionalLightResource_->direction.z);
					if (length != 0) {
						directionalLightResource_->direction.x /= length;
						directionalLightResource_->direction.y /= length;
						directionalLightResource_->direction.z /= length;
					}
				}

				ImGui::DragFloat("Intensity", &directionalLightResource_->intensity, 0.01f, 0.0f, 10.0f);

			}

			// スプライト設定
			if (ImGui::CollapsingHeader("Sprite Transform")) {
				ImGui::DragFloat3("Sprite Position", &sprite_->transform.translate.x, 1.0f);
				ImGui::DragFloat3("Sprite Rotation", &sprite_->transform.rotate.x, 0.01f);
				ImGui::DragFloat3("Sprite Scale", &sprite_->transform.scale.x, 0.01f);
			}

			if (ImGui::CollapsingHeader("UV Transform")) {
				ImGui::DragFloat3("UVTranslate", &sprite_->uvTransform.translate.x, 0.01f, -10.0f, 10.0f);
				ImGui::DragFloat3("UVScale", &sprite_->uvTransform.scale.x, 0.01f, -10.0f, 10.0f);
				ImGui::SliderAngle("UVRotate", &sprite_->uvTransform.rotate.z, -360.0f, 360.0f);
			}


			// モデル設定
			if (ImGui::CollapsingHeader("Axis Model Transform")) {
				ImGui::DragFloat3("Model Scale", &transform_.scale.x, 0.01f);
				ImGui::DragFloat3("Model Rotate", &transform_.rotate.x, 0.01f);
				ImGui::DragFloat3("Model Translate", &transform_.translate.x, 0.1f);
			}

			// 球の設定
			if (ImGui::CollapsingHeader("Sphere Transform")) {
				ImGui::DragFloat3("Sphere Scale", &sphereTransform_.scale.x, 0.01f);
				ImGui::DragFloat3("Sphere Rotate", &sphereTransform_.rotate.x, 0.01f);
				ImGui::DragFloat3("Sphere Translate", &sphereTransform_.translate.x, 0.1f);
			}

			break;
		case DrawMode::MultiMesh:

			// ライト設定
			if (ImGui::CollapsingHeader("Light Settings")) {

				const char* lightingModes[] = { "None", "Lambert", "Half-Lambert" };
				int currentMode = static_cast<int>(materialResourceSprite_->lightingMode);

				if (ImGui::Combo("Lighting Mode", &currentMode, lightingModes, IM_ARRAYSIZE(lightingModes))) {
					materialResourceSprite_->lightingMode = static_cast<float>(currentMode);
				}

				ImGui::Separator();

				ImGui::ColorEdit4("Light Color", &directionalLightResource_->color.x);

				if (ImGui::DragFloat3("Light Direction", &directionalLightResource_->direction.x, 0.01f, -1.0f, 1.0f)) {
					float length = std::sqrt(directionalLightResource_->direction.x * directionalLightResource_->direction.x +
						directionalLightResource_->direction.y * directionalLightResource_->direction.y +
						directionalLightResource_->direction.z * directionalLightResource_->direction.z);
					if (length != 0) {
						directionalLightResource_->direction.x /= length;
						directionalLightResource_->direction.y /= length;
						directionalLightResource_->direction.z /= length;
					}
				}

				ImGui::DragFloat("Intensity", &directionalLightResource_->intensity, 0.01f, 0.0f, 10.0f);

			}

			if (ImGui::CollapsingHeader("MultiMesh Transform")) {
				ImGui::DragFloat3("MultiMesh Scale", &multiMeshTransform_.scale.x, 0.01f);
				ImGui::DragFloat3("MultiMesh Rotate", &multiMeshTransform_.rotate.x, 0.01f);
				ImGui::DragFloat3("MultiMesh Translate", &multiMeshTransform_.translate.x, 0.1f);
			}

			break;

		case DrawMode::Sound:
			// サウンド設定
			if (ImGui::CollapsingHeader("Sound Settings")) {

				if (ImGui::Button("Play Sound")) {

					SoundPlayWave(xAudio2, &soundData1_);

				}

			}
			break;

		}


		//ImGui::Checkbox("Debug Camera", &isDebugCameraActive_);
		ImGui::End();
#endif
	}

	void GameScene::Draw(ID3D12GraphicsCommandList* commandList) {

		commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite_.GetGPUVirtualAddress());
		commandList->SetGraphicsRootConstantBufferView(2, directionalLightResource_.GetGPUVirtualAddress());

		// モードによる描画の切り替え
		switch (currentDrawMode_) {
		case DrawMode::Default:
			// axis.obj
			commandList->SetGraphicsRootConstantBufferView(1, wvpResource_.GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(3, textureSrvHandleGPU_);
			model_->Draw(commandList, &textureManager_);

			// sphere
			commandList->SetGraphicsRootConstantBufferView(1, sphereWvpResource_.GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(3, sphereTextureSrvHandleGPU_);
			sphereModel_->Draw(commandList, &textureManager_);

			// --- スプライトの描画 ---
			sprite_->Draw(commandList, directionalLightResource_.GetResource());

			break;

		case DrawMode::MultiMesh:

			// --- multiMesh.objの描画 ---
			commandList->SetGraphicsRootConstantBufferView(1, multiMeshWvpResource_.GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(3, multiMeshTextureSrvHandleGPU_);
			multiMeshModel_->Draw(commandList, &textureManager_);

			break;
		}

	}

	void GameScene::Finalize() {
		SoundUnload(&soundData1_);
	}

	std::vector<VertexData> GameScene::MakeSphere(uint32_t subdivision) {
		std::vector<VertexData> vertices;
		vertices.resize(subdivision * subdivision * 6);

		const float pi = 3.14159265358979f;
		const float kLonEvery = 2.0f * pi / float(subdivision);
		const float kLatEvery = pi / float(subdivision);

		auto SpherePos = [](float lat, float lon) {
			return Vector3(
				std::cos(lat) * std::cos(lon),
				std::sin(lat),
				std::cos(lat) * std::sin(lon)
			);
			};

		for (uint32_t latIndex = 0; latIndex < subdivision; ++latIndex) {
			float lat = -pi / 2.0f + kLatEvery * latIndex;

			for (uint32_t lonIndex = 0; lonIndex < subdivision; ++lonIndex) {
				float lon = lonIndex * kLonEvery;
				uint32_t start = (latIndex * subdivision + lonIndex) * 6;

				float u0 = float(lonIndex) / float(subdivision);
				float u1 = float(lonIndex + 1) / float(subdivision);
				float v0 = 1.0f - float(latIndex) / float(subdivision);
				float v1 = 1.0f - float(latIndex + 1) / float(subdivision);

				Vector3 a = SpherePos(lat, lon);
				Vector3 b = SpherePos(lat + kLatEvery, lon);
				Vector3 c = SpherePos(lat, lon + kLonEvery);
				Vector3 d = SpherePos(lat + kLatEvery, lon + kLonEvery);

				vertices[start + 0] = { Vector4(a.x, a.y, a.z, 1.0f), Vector2(u0, v0), a };
				vertices[start + 1] = { Vector4(b.x, b.y, b.z, 1.0f), Vector2(u0, v1), b };
				vertices[start + 2] = { Vector4(c.x, c.y, c.z, 1.0f), Vector2(u1, v0), c };

				vertices[start + 3] = { Vector4(c.x, c.y, c.z, 1.0f), Vector2(u1, v0), c };
				vertices[start + 4] = { Vector4(b.x, b.y, b.z, 1.0f), Vector2(u0, v1), b };
				vertices[start + 5] = { Vector4(d.x, d.y, d.z, 1.0f), Vector2(u1, v1), d };
			}
		}
		return vertices;
	}

}