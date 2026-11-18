#include "Player.h"

#include <cmath>

#include "MTEngine/Engine/Input/Input.h"
#include "MTEngine/Engine/Scene/Scene.h"
#include "MTEngine/Engine/Scene/SceneObject.h"

bool Player::Initialize(
    MTEngine::Scene& scene,
    MTEngine::Input& input) {

    scene_ = &scene;
    input_ = &input;

    for (const MTEngine::SceneObject& object : scene.GetObjects()) {
        if (object.name == "Player") {
            objectId_ = object.id;
            return true;
        }
    }

    return false;
}

void Player::Update(float deltaTime) {
    if (!scene_ || !input_) {
        return;
    }

    MTEngine::SceneObject* object =
        scene_->FindObject(objectId_);

    if (!object) {
        return;
    }

    float moveX = 0.0f;
    float moveZ = 0.0f;

    // キーボード
    if (input_->PushKey(DIK_W)) {
        moveZ += 1.0f;
    }
    if (input_->PushKey(DIK_S)) {
        moveZ -= 1.0f;
    }
    if (input_->PushKey(DIK_D)) {
        moveX += 1.0f;
    }
    if (input_->PushKey(DIK_A)) {
        moveX -= 1.0f;
    }

    // コントローラー
    const MTEngine::GamePad* gamePad =
        input_->GetGamePad();

    if (gamePad && gamePad->IsConnected()) {
        const MTEngine::StickState& stick =
            gamePad->GetLeftStick();

        moveX += stick.x;
        moveZ += stick.y;
    }

    // 斜め移動が速くならないよう正規化
    const float length =
        std::sqrt(moveX * moveX + moveZ * moveZ);

    if (length > 1.0f) {
        moveX /= length;
        moveZ /= length;
    }

    object->transform.translate.x +=
        moveX * kMoveSpeed * deltaTime;

    object->transform.translate.z +=
        moveZ * kMoveSpeed * deltaTime;

    // 移動方向へ回転
    if (length > 0.001f) {
        object->transform.rotate.y =
            std::atan2(moveX, moveZ);
    }
}

const MTEngine::SceneObject*
Player::GetSceneObject() const {
    if (!scene_) {
        return nullptr;
    }

    return scene_->FindObject(objectId_);
}
