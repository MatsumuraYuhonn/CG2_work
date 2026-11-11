#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <string>

#include "MTEngine/Engine/Input/Input.h"
#include "MTEngine/Engine/Particle/ParticleEffectData.h"
#include "MTEngine/Engine/Particle/ParticleSystem.h"
#include "MTEngine/Engine/Scene/Scene.h"

class MeteorEffect {
public:
    enum class State {
        Falling,
        Exploding,
        Burning,
        Waiting
    };

    static MTEngine::ParticleEffectData CreateTrailData() {
        MTEngine::ParticleEffectData data;
        data.emitter.spawnArea = { 0.6f, 0.6f, 0.6f };
        data.emitter.spawnRate = kTrailSpawnRate;
        data.emitter.maxParticles = 160;
        data.emitter.loop = true;
        data.particle.lifeTime = { 0.35f, 0.75f };
        data.particle.velocity = {
            { -0.3f, 0.5f, -0.3f },
            { 0.3f, 1.5f, 0.3f }
        };
        data.particle.acceleration = { 0.0f, 0.5f, 0.0f };
        data.particle.startColor = { 1.0f, 0.25f, 0.02f, 1.0f };
        data.particle.endColor = { 0.2f, 0.01f, 0.0f, 0.0f };
        data.particle.startSize = 0.55f;
        data.particle.endSize = 0.08f;
        data.render.texturePath =
            "MTEngine/Assets/Resources/ball/fireball.png";
        return data;
    }

    bool Initialize(
        MTEngine::Scene& scene,
        MTEngine::ParticleSystem& particleSystem,
        MTEngine::ParticleSystem::EmitterId trailEmitterId) {
        scene_ = &scene;
        particleSystem_ = &particleSystem;
        trailEmitterId_ = trailEmitterId;

        for (const MTEngine::SceneObject& object : scene.GetObjects()) {
            if (object.modelPath.find("ball/ball.obj") !=
                std::string::npos) {
                meteorObjectId_ = object.id;
            }
            if (object.modelPath.find("plane.obj") !=
                std::string::npos) {
                planeHeight_ = object.transform.translate.y;
                planeCenterX_ = object.transform.translate.x;
                planeCenterZ_ = object.transform.translate.z;
                planeHalfExtentX_ = std::abs(
                    object.transform.scale.x);
                planeHalfExtentZ_ = std::abs(
                    object.transform.scale.y);
            }
        }

        MTEngine::SceneObject* meteorObject = GetMeteorObject();
        if (!meteorObject || trailEmitterId_ == 0) {
            return false;
        }

        initialTransform_ = meteorObject->transform;
        explosionEmitterId_ = particleSystem_->CreateEmitter(
            CreateExplosionData());
        fireEmitterId_ = particleSystem_->CreateEmitter(
            CreateFireData());
        if (explosionEmitterId_ == 0 || fireEmitterId_ == 0) {
            return false;
        }

        Reset();
        return true;
    }

    void Update(float deltaTime, const MTEngine::Input& input) {
        if (!scene_ || !particleSystem_) {
            return;
        }

        if (input.TriggerKey(DIK_R)) {
            Reset();
        }

        if (state_ == State::Falling) {
            UpdateFalling(deltaTime);
            return;
        }

        if (state_ == State::Exploding) {
            stateElapsed_ += deltaTime;
            if (stateElapsed_ >= kFireStartDelay) {
                StartFire();
            }
            return;
        }

        if (state_ == State::Burning) {
            stateElapsed_ += deltaTime;
            if (stateElapsed_ >= kFireDuration) {
                particleSystem_->SetEmitterSpawnRate(
                    fireEmitterId_,
                    0.0f);
                state_ = State::Waiting;
                stateElapsed_ = 0.0f;
            }
            return;
        }

        if (state_ == State::Waiting) {
            stateElapsed_ += deltaTime;
            if (stateElapsed_ >= kNextMeteorDelay) {
                Reset();
            }
        }
    }

    void RestoreSceneObject() {
        MTEngine::SceneObject* meteorObject = GetMeteorObject();
        if (meteorObject) {
            meteorObject->transform = initialTransform_;
        }
    }

    MTEngine::Vector3 GetFocusPosition() const {
        const MTEngine::SceneObject* meteorObject = GetMeteorObject();
        if (state_ == State::Falling && meteorObject) {
            return meteorObject->transform.translate;
        }
        return impactPosition_;
    }

    bool IsInitialized() const {
        return scene_ != nullptr && meteorObjectId_ != 0;
    }

    State GetState() const {
        return state_;
    }

private:
    static MTEngine::ParticleEffectData CreateExplosionData() {
        MTEngine::ParticleEffectData data;
        data.emitter.spawnArea = { 0.0f, 0.0f, 0.0f };
        data.emitter.spawnRate = 0.0f;
        data.emitter.emissionDuration = 0.01f;
        data.emitter.maxParticles = 200;
        data.emitter.loop = false;
        data.particle.lifeTime = { 0.35f, 0.9f };
        data.particle.acceleration = { 0.0f, -4.0f, 0.0f };
        data.particle.startColor = { 1.0f, 0.8f, 0.1f, 1.0f };
        data.particle.endColor = { 1.0f, 0.05f, 0.0f, 0.0f };
        data.particle.startSize = 0.65f;
        data.particle.endSize = 0.04f;
        data.render.texturePath =
            "MTEngine/Assets/Resources/circle.png";
        return data;
    }

    static MTEngine::ParticleEffectData CreateFireData() {
        MTEngine::ParticleEffectData data;
        data.emitter.spawnArea = { 1.8f, 0.2f, 1.8f };
        data.emitter.spawnRate = kFireSpawnRate;
        data.emitter.maxParticles = 140;
        data.emitter.loop = true;
        data.particle.lifeTime = { 0.55f, 1.2f };
        data.particle.velocity = {
            { -0.35f, 1.2f, -0.35f },
            { 0.35f, 3.2f, 0.35f }
        };
        data.particle.acceleration = { 0.0f, 0.4f, 0.0f };
        data.particle.startColor = { 1.0f, 0.35f, 0.02f, 1.0f };
        data.particle.endColor = { 0.35f, 0.01f, 0.0f, 0.0f };
        data.particle.startSize = 0.55f;
        data.particle.endSize = 0.05f;
        data.render.texturePath =
            "MTEngine/Assets/Resources/ball/fireball.png";
        return data;
    }

    void UpdateFalling(float deltaTime) {
        MTEngine::SceneObject* meteorObject = GetMeteorObject();
        if (!meteorObject) {
            return;
        }

        meteorVelocity_.y += kGravity * deltaTime;
        meteorObject->transform.translate.x +=
            meteorVelocity_.x * deltaTime;
        meteorObject->transform.translate.y +=
            meteorVelocity_.y * deltaTime;
        meteorObject->transform.translate.z +=
            meteorVelocity_.z * deltaTime;
        meteorObject->transform.rotate.x +=
            kRotationSpeed * deltaTime;
        meteorObject->transform.rotate.z +=
            kRotationSpeed * 0.7f * deltaTime;

        particleSystem_->SetEmitterPosition(
            trailEmitterId_,
            meteorObject->transform.translate);

        const float radius =
            kBaseMeteorRadius * meteorObject->transform.scale.y;
        if (meteorObject->transform.translate.y - radius <=
            planeHeight_) {
            impactPosition_ = meteorObject->transform.translate;
            impactPosition_.y = planeHeight_ + kEffectHeightOffset;
            StartExplosion();
        }
    }

    void StartExplosion() {
        MTEngine::SceneObject* meteorObject = GetMeteorObject();
        if (!meteorObject) {
            return;
        }

        meteorObject->transform.scale = { 0.0f, 0.0f, 0.0f };
        particleSystem_->SetEmitterSpawnRate(trailEmitterId_, 0.0f);

        particleSystem_->SetEmitterPosition(
            explosionEmitterId_,
            impactPosition_);
        particleSystem_->Restart(explosionEmitterId_);
        particleSystem_->EmitRadialBurst(
            explosionEmitterId_,
            kExplosionParticleCount,
            kExplosionMinimumSpeed,
            kExplosionMaximumSpeed,
            kExplosionMinimumDirectionY);

        state_ = State::Exploding;
        stateElapsed_ = 0.0f;
    }

    void StartFire() {
        particleSystem_->SetEmitterPosition(
            fireEmitterId_,
            impactPosition_);
        particleSystem_->Restart(fireEmitterId_);
        state_ = State::Burning;
        stateElapsed_ = 0.0f;
    }

    void Reset() {
        MTEngine::SceneObject* meteorObject = GetMeteorObject();
        if (!meteorObject || !particleSystem_) {
            return;
        }

        meteorObject->transform = initialTransform_;
        const float horizontalMargin =
            kPlaneEdgeMargin +
            kBaseMeteorRadius * meteorObject->transform.scale.x;
        const float availableHalfExtentX = (std::max)(
            0.0f,
            planeHalfExtentX_ - horizontalMargin);
        const float availableHalfExtentZ = (std::max)(
            0.0f,
            planeHalfExtentZ_ - horizontalMargin);
        meteorObject->transform.translate = {
            RandomRange(
                planeCenterX_ - availableHalfExtentX,
                planeCenterX_ + availableHalfExtentX),
            planeHeight_ + kMeteorSpawnHeight,
            RandomRange(
                planeCenterZ_ - availableHalfExtentZ,
                planeCenterZ_ + availableHalfExtentZ)
        };
        meteorVelocity_ = kInitialVelocity;
        impactPosition_ = meteorObject->transform.translate;
        state_ = State::Falling;
        stateElapsed_ = 0.0f;

        particleSystem_->Restart(trailEmitterId_);
        particleSystem_->SetEmitterSpawnRate(
            trailEmitterId_,
            kTrailSpawnRate);
        particleSystem_->SetEmitterPosition(
            trailEmitterId_,
            meteorObject->transform.translate);

        particleSystem_->Restart(explosionEmitterId_);
        particleSystem_->SetPlaying(explosionEmitterId_, false);
        particleSystem_->Restart(fireEmitterId_);
        particleSystem_->SetEmitterSpawnRate(
            fireEmitterId_,
            kFireSpawnRate);
        particleSystem_->SetPlaying(fireEmitterId_, false);
    }

    float RandomRange(float minimum, float maximum) {
        if (minimum > maximum) {
            std::swap(minimum, maximum);
        }
        std::uniform_real_distribution<float> distribution(
            minimum,
            maximum);
        return distribution(randomEngine_);
    }

    MTEngine::SceneObject* GetMeteorObject() {
        if (!scene_) {
            return nullptr;
        }
        return scene_->FindObject(meteorObjectId_);
    }

    const MTEngine::SceneObject* GetMeteorObject() const {
        if (!scene_) {
            return nullptr;
        }
        return scene_->FindObject(meteorObjectId_);
    }

    static constexpr float kGravity = -9.8f;
    static constexpr float kBaseMeteorRadius = 0.5f;
    static constexpr float kRotationSpeed = 2.5f;
    static constexpr float kEffectHeightOffset = 0.12f;
    static constexpr float kFireStartDelay = 0.18f;
    static constexpr float kFireDuration = 3.0f;
    static constexpr float kNextMeteorDelay = 2.0f;
    static constexpr float kMeteorSpawnHeight = 8.0f;
    static constexpr float kPlaneEdgeMargin = 0.5f;
    static constexpr float kTrailSpawnRate = 110.0f;
    static constexpr float kFireSpawnRate = 70.0f;
    static constexpr uint32_t kExplosionParticleCount = 150;
    static constexpr float kExplosionMinimumSpeed = 5.0f;
    static constexpr float kExplosionMaximumSpeed = 12.0f;
    static constexpr float kExplosionMinimumDirectionY = 0.05f;
    static constexpr MTEngine::Vector3 kInitialVelocity = {
        0.0f, -3.0f, 0.0f
    };

    MTEngine::Scene* scene_ = nullptr;
    MTEngine::ParticleSystem* particleSystem_ = nullptr;
    MTEngine::Transform initialTransform_{};
    MTEngine::Vector3 meteorVelocity_{};
    MTEngine::Vector3 impactPosition_{};
    MTEngine::ParticleSystem::EmitterId trailEmitterId_ = 0;
    MTEngine::ParticleSystem::EmitterId explosionEmitterId_ = 0;
    MTEngine::ParticleSystem::EmitterId fireEmitterId_ = 0;
    uint64_t meteorObjectId_ = 0;
    float planeHeight_ = 0.0f;
    float planeCenterX_ = 0.0f;
    float planeCenterZ_ = 0.0f;
    float planeHalfExtentX_ = 0.0f;
    float planeHalfExtentZ_ = 0.0f;
    float stateElapsed_ = 0.0f;
    State state_ = State::Falling;
    std::mt19937 randomEngine_{ std::random_device{}() };
};
