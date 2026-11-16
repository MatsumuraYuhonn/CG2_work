#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

#include "MTEngine/Engine/Math/Vector.h"

namespace MTEngine {

    struct ParticleFloatRange {
        float min = 1.0f;
        float max = 1.0f;
    };

    struct ParticleVector3Range {
        Vector3 min{};
        Vector3 max{};
    };

    enum class ParticleBlendMode {
        Alpha,
        Additive
    };

    struct ParticleEmitterSettings {
        Vector3 position{};
        Vector3 spawnArea{ 1.0f, 1.0f, 1.0f };
        float spawnRate = 30.0f;
        float emissionDuration = 2.0f;
        uint32_t maxParticles = 500;
        bool loop = true;
    };

    struct ParticleMotionSettings {
        ParticleFloatRange lifeTime{ 0.8f, 1.5f };
        ParticleVector3Range velocity{
            { -0.5f, 1.0f, -0.5f },
            { 0.5f, 3.0f, 0.5f }
        };
        Vector3 acceleration{ 0.0f, -0.5f, 0.0f };
        Vector4 startColor{ 1.0f, 0.5f, 0.1f, 1.0f };
        Vector4 endColor{ 0.3f, 0.05f, 0.0f, 0.0f };
        float startSize = 0.5f;
        float endSize = 0.0f;
    };

    struct ParticleRenderSettings {
        std::string texturePath =
            "MTEngine/Assets/Resources/uvChecker.png";
        ParticleBlendMode blendMode = ParticleBlendMode::Additive;
        bool billboard = true;
    };

    struct ParticleEffectData {
        static constexpr int kCurrentVersion = 1;
        static constexpr uint32_t kMaximumParticleLimit = 4096;

        int version = kCurrentVersion;
        std::string name = "Fire";
        ParticleEmitterSettings emitter;
        ParticleMotionSettings particle;
        ParticleRenderSettings render;
    };

    inline void ValidateParticleEffectData(ParticleEffectData& data) {
        data.version = ParticleEffectData::kCurrentVersion;
        data.emitter.spawnRate =
            (std::max)(0.0f, data.emitter.spawnRate);
        data.emitter.emissionDuration =
            (std::max)(0.01f, data.emitter.emissionDuration);
        data.emitter.maxParticles = (std::clamp)(
            data.emitter.maxParticles,
            1u,
            ParticleEffectData::kMaximumParticleLimit);

        data.emitter.spawnArea.x =
            (std::max)(0.0f, data.emitter.spawnArea.x);
        data.emitter.spawnArea.y =
            (std::max)(0.0f, data.emitter.spawnArea.y);
        data.emitter.spawnArea.z =
            (std::max)(0.0f, data.emitter.spawnArea.z);

        data.particle.lifeTime.min =
            (std::max)(0.01f, data.particle.lifeTime.min);
        data.particle.lifeTime.max =
            (std::max)(data.particle.lifeTime.min,
                data.particle.lifeTime.max);

        data.particle.startSize =
            (std::max)(0.0f, data.particle.startSize);
        data.particle.endSize =
            (std::max)(0.0f, data.particle.endSize);
    }

}
