#include "PlayerBulletManager.h"

#include <cmath>

#include "MTEngine/Engine/Scene/Scene.h"
#include "MTEngine/Engine/Scene/SceneObject.h"

bool PlayerBulletManager::Initialize(
    MTEngine::Scene& scene) {

    scene_ = &scene;
    fireTimer_ = 0.0f;
    bullets_.clear();

    return true;
}

void PlayerBulletManager::Update(
    float deltaTime,
    const MTEngine::Vector3& playerPosition,
    const MTEngine::Vector3& aimDirection) {

    if (!scene_) {
        return;
    }

    fireTimer_ -= deltaTime;

    if (fireTimer_ <= 0.0f) {
        Fire(playerPosition, aimDirection);
        fireTimer_ = kFireInterval;
    }

    UpdateBullets(deltaTime);
}

void PlayerBulletManager::Fire(
    const MTEngine::Vector3& playerPosition,
    const MTEngine::Vector3& aimDirection) {

    if (!scene_) {
        return;
    }

    const float directionLength = std::sqrt(
        aimDirection.x * aimDirection.x +
        aimDirection.y * aimDirection.y +
        aimDirection.z * aimDirection.z);

    if (directionLength <= 0.001f) {
        return;
    }

    MTEngine::Vector3 direction{};
    direction.x = aimDirection.x / directionLength;
    direction.y = aimDirection.y / directionLength;
    direction.z = aimDirection.z / directionLength;

    const uint64_t objectId = scene_->CreateObject(
        "PlayerBullet",
        "MTEngine/Assets/Resources/block/block.obj");

    MTEngine::SceneObject* object =
        scene_->FindObject(objectId);

    if (!object) {
        return;
    }

    object->transform.translate.x =
        playerPosition.x + direction.x * kSpawnDistance;

    object->transform.translate.y =
        playerPosition.y + 0.5f;

    object->transform.translate.z =
        playerPosition.z + direction.z * kSpawnDistance;

    object->transform.scale = {
        0.2f,
        0.2f,
        0.2f
    };

    Bullet bullet;
    bullet.objectId = objectId;
    bullet.direction = direction;
    bullet.remainingLife = kBulletLifeTime;

    bullets_.push_back(bullet);
}

void PlayerBulletManager::UpdateBullets(
    float deltaTime) {

    if (!scene_) {
        return;
    }

    auto iterator = bullets_.begin();

    while (iterator != bullets_.end()) {
        MTEngine::SceneObject* object =
            scene_->FindObject(iterator->objectId);

        iterator->remainingLife -= deltaTime;

        if (!object ||
            iterator->remainingLife <= 0.0f) {

            scene_->RemoveObject(iterator->objectId);
            iterator = bullets_.erase(iterator);
            continue;
        }

        object->transform.translate.x +=
            iterator->direction.x *
            kBulletSpeed *
            deltaTime;

        object->transform.translate.y +=
            iterator->direction.y *
            kBulletSpeed *
            deltaTime;

        object->transform.translate.z +=
            iterator->direction.z *
            kBulletSpeed *
            deltaTime;

        ++iterator;
    }
}

void PlayerBulletManager::Clear() {
    if (scene_) {
        for (const Bullet& bullet : bullets_) {
            scene_->RemoveObject(bullet.objectId);
        }
    }

    bullets_.clear();
    fireTimer_ = 0.0f;
}