#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <random>
#include <string>
#include <vector>

#include <d3d12.h>
#include <wrl.h>

#include "MTEngine/Engine/Debug/DebugCamera.h"
#include "MTEngine/Engine/Graphics/DescriptorHeapManager.h"
#include "MTEngine/Engine/Graphics/Model.h"
#include "MTEngine/Engine/Graphics/Renderer.h"
#include "MTEngine/Engine/Graphics/Sprite.h"
#include "MTEngine/Engine/Graphics/TextureManager.h"
#include "MTEngine/Engine/Math/Matrix.h"
#include "MTEngine/Engine/Particle/ParticleEffectData.h"

namespace MTEngine {

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(
        Microsoft::WRL::ComPtr<ID3D12Device> device,
        size_t sizeInBytes);

    class ParticleSystem {
    public:
        using EmitterId = uint64_t;

        bool Initialize(
            ID3D12Device* device,
            DescriptorHeapManager* srvHeapManager,
            ID3D12GraphicsCommandList* commandList,
            const ParticleEffectData& settings) {

            if (!device || !srvHeapManager || !commandList) {
                lastError_ = "ParticleSystem initialization failed";
                return false;
            }

            device_ = device;
            commandList_ = commandList;
            textureManager_.Initialize(device_, srvHeapManager);

            settings_ = settings;
            ValidateParticleEffectData(settings_);

            if (!CreateGeometry()) {
                lastError_ = "Failed to create particle geometry";
                return false;
            }
            if (!CreateInstanceBuffers(settings_.emitter.maxParticles)) {
                lastError_ = "Failed to create particle instance buffers";
                return false;
            }
            D3D12_GPU_DESCRIPTOR_HANDLE textureHandle{};
            if (!LoadTexture(
                settings_.render.texturePath,
                textureHandle)) {
                return false;
            }

            particles_.reserve(settings_.emitter.maxParticles);
            Emitter emitter;
            emitter.id = nextEmitterId_++;
            emitter.settings = settings_;
            emitter.textureHandle = textureHandle;
            emitters_.push_back(emitter);
            bufferParticleCapacity_ = settings_.emitter.maxParticles;
            isInitialized_ = true;
            lastError_.clear();
            return true;
        }

        bool ApplySettings(const ParticleEffectData& source) {
            if (emitters_.empty()) {
                lastError_ = "Particle emitter is not available";
                return false;
            }
            return ApplySettings(emitters_.front().id, source);
        }

        bool ApplySettings(
            EmitterId emitterId,
            const ParticleEffectData& source) {
            if (!isInitialized_) {
                return false;
            }

            Emitter* emitter = FindEmitter(emitterId);
            if (!emitter) {
                lastError_ = "Particle emitter is not available";
                return false;
            }

            ParticleEffectData settings = source;
            ValidateParticleEffectData(settings);

            D3D12_GPU_DESCRIPTOR_HANDLE textureHandle =
                emitter->textureHandle;
            if (settings.render.texturePath !=
                emitter->settings.render.texturePath) {
                if (!LoadTexture(
                    settings.render.texturePath,
                    textureHandle)) {
                    return false;
                }
            }

            const uint32_t requiredCapacity = CalculateRequiredCapacity(
                emitterId,
                settings.emitter.maxParticles);
            if (requiredCapacity == 0) {
                lastError_ = "Particle capacity is too large";
                return false;
            }

            if (requiredCapacity > bufferParticleCapacity_) {
                if (!CreateInstanceBuffers(requiredCapacity)) {
                    lastError_ = "Failed to resize particle instance buffers";
                    return false;
                }
                bufferParticleCapacity_ = requiredCapacity;
                particles_.reserve(requiredCapacity);
            }

            emitter->settings = settings;
            emitter->textureHandle = textureHandle;
            TrimEmitterParticles(
                emitterId,
                settings.emitter.maxParticles);
            lastError_.clear();
            return true;
        }

        EmitterId CreateEmitter(
            const ParticleEffectData& source) {
            if (!isInitialized_) {
                lastError_ = "ParticleSystem is not initialized";
                return 0;
            }

            ParticleEffectData validatedData = source;
            ValidateParticleEffectData(validatedData);

            D3D12_GPU_DESCRIPTOR_HANDLE textureHandle{};
            if (!LoadTexture(
                validatedData.render.texturePath,
                textureHandle)) {
                return 0;
            }

            const uint32_t requiredCapacity = CalculateRequiredCapacity(
                0,
                validatedData.emitter.maxParticles);
            if (requiredCapacity == 0) {
                lastError_ = "Particle capacity is too large";
                return 0;
            }

            if (requiredCapacity > bufferParticleCapacity_) {
                if (!CreateInstanceBuffers(requiredCapacity)) {
                    lastError_ = "Failed to resize particle instance buffers";
                    return 0;
                }
                bufferParticleCapacity_ = requiredCapacity;
                particles_.reserve(requiredCapacity);
            }

            Emitter emitter;
            emitter.id = nextEmitterId_++;
            emitter.settings = validatedData;
            emitter.textureHandle = textureHandle;
            emitters_.push_back(emitter);
            lastError_.clear();
            return emitter.id;
        }

        bool RemoveEmitter(EmitterId emitterId) {
            const auto emitterIterator = std::find_if(
                emitters_.begin(),
                emitters_.end(),
                [emitterId](const Emitter& emitter) {
                    return emitter.id == emitterId;
                });
            if (emitterIterator == emitters_.end()) {
                return false;
            }

            const auto particleIterator = std::remove_if(
                particles_.begin(),
                particles_.end(),
                [emitterId](const Particle& particle) {
                    return particle.emitterId == emitterId;
                });
            particles_.erase(particleIterator, particles_.end());
            emitters_.erase(emitterIterator);
            return true;
        }

        std::vector<EmitterId> GetEmitterIds() const {
            std::vector<EmitterId> emitterIds;
            emitterIds.reserve(emitters_.size());
            for (const Emitter& emitter : emitters_) {
                emitterIds.push_back(emitter.id);
            }
            return emitterIds;
        }

        const ParticleEffectData* GetEmitterData(
            EmitterId emitterId) const {
            const Emitter* emitter = FindEmitter(emitterId);
            if (!emitter) {
                return nullptr;
            }
            return &emitter->settings;
        }

        void Update(float deltaTime) {
            if (!isInitialized_ || !isPlaying_) {
                return;
            }

            deltaTime = (std::clamp)(deltaTime, 0.0f, 0.1f);
            for (Emitter& emitter : emitters_) {
                if (!emitter.isPlaying) {
                    continue;
                }

                emitter.emissionElapsed += deltaTime;
                bool canEmit = emitter.settings.emitter.loop;
                if (!emitter.settings.emitter.loop) {
                    canEmit = emitter.emissionElapsed <=
                        emitter.settings.emitter.emissionDuration;
                }

                if (!canEmit ||
                    emitter.settings.emitter.spawnRate <= 0.0f) {
                    continue;
                }

                emitter.spawnAccumulator +=
                    emitter.settings.emitter.spawnRate * deltaTime;
                const uint32_t spawnCount = static_cast<uint32_t>(
                    emitter.spawnAccumulator);
                emitter.spawnAccumulator -=
                    static_cast<float>(spawnCount);

                size_t activeParticleCount =
                    GetActiveParticleCount(emitter.id);
                for (uint32_t index = 0; index < spawnCount; ++index) {
                    if (activeParticleCount >=
                        emitter.settings.emitter.maxParticles) {
                        break;
                    }
                    SpawnParticle(emitter);
                    ++activeParticleCount;
                }
            }

            for (Particle& particle : particles_) {
                const Emitter* emitter = FindEmitter(particle.emitterId);
                if (!emitter || !emitter->isPlaying) {
                    continue;
                }
                particle.age += deltaTime;
                particle.velocity.x +=
                    emitter->settings.particle.acceleration.x * deltaTime;
                particle.velocity.y +=
                    emitter->settings.particle.acceleration.y * deltaTime;
                particle.velocity.z +=
                    emitter->settings.particle.acceleration.z * deltaTime;
                particle.position.x += particle.velocity.x * deltaTime;
                particle.position.y += particle.velocity.y * deltaTime;
                particle.position.z += particle.velocity.z * deltaTime;
            }

            const auto expiredParticle = std::remove_if(
                particles_.begin(),
                particles_.end(),
                [](const Particle& particle) {
                    return particle.age >= particle.lifeTime;
                });
            particles_.erase(expiredParticle, particles_.end());
        }

        void Draw(Renderer* renderer, const DebugCamera& camera) {
            if (!isInitialized_ || !renderer || particles_.empty()) {
                return;
            }

            ID3D12GraphicsCommandList* commandList =
                renderer->GetCommandList();
            const D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[2] = {
                vertexBufferView_,
                instanceBufferView_
            };
            commandList->IASetVertexBuffers(0, 2, vertexBufferViews);
            commandList->IASetIndexBuffer(&indexBufferView_);

            Matrix4x4 billboardMatrix =
                Inverse(camera.GetViewMatrix());
            billboardMatrix.m[3][0] = 0.0f;
            billboardMatrix.m[3][1] = 0.0f;
            billboardMatrix.m[3][2] = 0.0f;

            const Matrix4x4 viewProjection = Multiply(
                camera.GetViewMatrix(),
                camera.GetProjectionMatrix());

            ParticleInstanceData* instanceData =
                static_cast<ParticleInstanceData*>(instanceMappedData_);
            ParticleCameraData* cameraData =
                static_cast<ParticleCameraData*>(cameraMappedData_);
            cameraData->viewProjection = viewProjection;

            UINT instanceIndex = 0;
            for (const Emitter& emitter : emitters_) {
                const UINT firstInstance = instanceIndex;
                for (const Particle& particle : particles_) {
                    if (particle.emitterId != emitter.id) {
                        continue;
                    }

                    float normalizedAge =
                        particle.age / particle.lifeTime;
                    normalizedAge = (std::clamp)(
                        normalizedAge,
                        0.0f,
                        1.0f);

                    const float size = Lerp(
                        emitter.settings.particle.startSize,
                        emitter.settings.particle.endSize,
                        normalizedAge);
                    const Vector4 color = Lerp(
                        emitter.settings.particle.startColor,
                        emitter.settings.particle.endColor,
                        normalizedAge);

                    Matrix4x4 world = MakeScaleMatrix(
                        { size, size, size });
                    if (emitter.settings.render.billboard) {
                        world = Multiply(world, billboardMatrix);
                    }
                    world = Multiply(
                        world,
                        MakeTranslateMatrix(particle.position));

                    instanceData[instanceIndex].world = world;
                    instanceData[instanceIndex].color = color;
                    ++instanceIndex;
                }

                const UINT instanceCount =
                    instanceIndex - firstInstance;
                if (instanceCount == 0) {
                    continue;
                }

                BlendMode blendMode = BlendMode::kBlendModeAdd;
                if (emitter.settings.render.blendMode ==
                    ParticleBlendMode::Alpha) {
                    blendMode = BlendMode::kBlendModeNormal;
                }
                renderer->SetParticlePipelineState(
                    blendMode,
                    DepthWriteMode::kDisable);
                commandList->SetGraphicsRootConstantBufferView(
                    0,
                    cameraResource_->GetGPUVirtualAddress());
                commandList->SetGraphicsRootDescriptorTable(
                    1,
                    emitter.textureHandle);
                commandList->DrawIndexedInstanced(
                    6,
                    instanceCount,
                    0,
                    0,
                    firstInstance);
            }

            renderer->SetPipelineState(
                BlendMode::kBlendModeNormal,
                DepthWriteMode::kEnable);
        }

        void Restart() {
            particles_.clear();
            for (Emitter& emitter : emitters_) {
                emitter.spawnAccumulator = 0.0f;
                emitter.emissionElapsed = 0.0f;
                emitter.isPlaying = true;
            }
            isPlaying_ = true;
        }

        bool Restart(EmitterId emitterId) {
            Emitter* emitter = FindEmitter(emitterId);
            if (!emitter) {
                return false;
            }

            const auto particleIterator = std::remove_if(
                particles_.begin(),
                particles_.end(),
                [emitterId](const Particle& particle) {
                    return particle.emitterId == emitterId;
                });
            particles_.erase(particleIterator, particles_.end());
            emitter->spawnAccumulator = 0.0f;
            emitter->emissionElapsed = 0.0f;
            emitter->isPlaying = true;
            return true;
        }

        void SetPlaying(bool isPlaying) {
            isPlaying_ = isPlaying;
        }

        bool IsPlaying() const {
            return isPlaying_;
        }

        bool SetPlaying(EmitterId emitterId, bool isPlaying) {
            Emitter* emitter = FindEmitter(emitterId);
            if (!emitter) {
                return false;
            }
            emitter->isPlaying = isPlaying;
            return true;
        }

        bool IsPlaying(EmitterId emitterId) const {
            const Emitter* emitter = FindEmitter(emitterId);
            if (!emitter) {
                return false;
            }
            return emitter->isPlaying;
        }

        size_t GetActiveParticleCount() const {
            return particles_.size();
        }

        size_t GetActiveParticleCount(EmitterId emitterId) const {
            return static_cast<size_t>(std::count_if(
                particles_.begin(),
                particles_.end(),
                [emitterId](const Particle& particle) {
                    return particle.emitterId == emitterId;
                }));
        }

        const ParticleEffectData& GetSettings() const {
            return settings_;
        }

        const std::string& GetLastError() const {
            return lastError_;
        }

    private:
        struct Particle {
            EmitterId emitterId = 0;
            Vector3 position{};
            Vector3 velocity{};
            float age = 0.0f;
            float lifeTime = 1.0f;
        };

        struct Emitter {
            EmitterId id = 0;
            ParticleEffectData settings;
            D3D12_GPU_DESCRIPTOR_HANDLE textureHandle{};
            float spawnAccumulator = 0.0f;
            float emissionElapsed = 0.0f;
            bool isPlaying = true;
        };

        struct ParticleInstanceData {
            Matrix4x4 world;
            Vector4 color;
        };

        struct ParticleCameraData {
            Matrix4x4 viewProjection;
        };

        static size_t AlignConstantBufferSize(size_t size) {
            return (size + 255) & ~static_cast<size_t>(255);
        }

        Emitter* FindEmitter(EmitterId emitterId) {
            const auto iterator = std::find_if(
                emitters_.begin(),
                emitters_.end(),
                [emitterId](const Emitter& emitter) {
                    return emitter.id == emitterId;
                });
            if (iterator == emitters_.end()) {
                return nullptr;
            }
            return &(*iterator);
        }

        const Emitter* FindEmitter(EmitterId emitterId) const {
            const auto iterator = std::find_if(
                emitters_.begin(),
                emitters_.end(),
                [emitterId](const Emitter& emitter) {
                    return emitter.id == emitterId;
                });
            if (iterator == emitters_.end()) {
                return nullptr;
            }
            return &(*iterator);
        }

        uint32_t CalculateRequiredCapacity(
            EmitterId replacedEmitterId,
            uint32_t replacementCapacity) const {
            uint64_t capacity = replacementCapacity;
            for (const Emitter& emitter : emitters_) {
                if (emitter.id == replacedEmitterId) {
                    continue;
                }
                capacity += emitter.settings.emitter.maxParticles;
            }

            const uint64_t maximumCapacity =
                static_cast<uint64_t>((std::numeric_limits<UINT>::max)()) /
                sizeof(ParticleInstanceData);
            if (capacity == 0 || capacity > maximumCapacity) {
                return 0;
            }
            return static_cast<uint32_t>(capacity);
        }

        void TrimEmitterParticles(
            EmitterId emitterId,
            uint32_t maximumParticles) {
            uint32_t keptParticleCount = 0;
            const auto iterator = std::remove_if(
                particles_.begin(),
                particles_.end(),
                [emitterId,
                 maximumParticles,
                 &keptParticleCount](const Particle& particle) {
                    if (particle.emitterId != emitterId) {
                        return false;
                    }
                    if (keptParticleCount < maximumParticles) {
                        ++keptParticleCount;
                        return false;
                    }
                    return true;
                });
            particles_.erase(iterator, particles_.end());
        }

        bool CreateGeometry() {
            const VertexData vertices[4] = {
                {{-0.5f,  0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
                {{ 0.5f,  0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
                {{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
                {{ 0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}
            };
            const uint32_t indices[6] = { 0, 1, 2, 1, 3, 2 };

            vertexResource_ = CreateBufferResource(
                device_, sizeof(vertices));
            indexResource_ = CreateBufferResource(
                device_, sizeof(indices));
            if (!vertexResource_ || !indexResource_) {
                return false;
            }

            void* mappedVertices = nullptr;
            vertexResource_->Map(0, nullptr, &mappedVertices);
            std::memcpy(mappedVertices, vertices, sizeof(vertices));
            vertexResource_->Unmap(0, nullptr);

            void* mappedIndices = nullptr;
            indexResource_->Map(0, nullptr, &mappedIndices);
            std::memcpy(mappedIndices, indices, sizeof(indices));
            indexResource_->Unmap(0, nullptr);

            vertexBufferView_.BufferLocation =
                vertexResource_->GetGPUVirtualAddress();
            vertexBufferView_.SizeInBytes = sizeof(vertices);
            vertexBufferView_.StrideInBytes = sizeof(VertexData);

            indexBufferView_.BufferLocation =
                indexResource_->GetGPUVirtualAddress();
            indexBufferView_.SizeInBytes = sizeof(indices);
            indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
            return true;
        }

        bool CreateInstanceBuffers(uint32_t maximumParticles) {
            if (instanceResource_ && instanceMappedData_) {
                instanceResource_->Unmap(0, nullptr);
                instanceMappedData_ = nullptr;
            }
            if (cameraResource_ && cameraMappedData_) {
                cameraResource_->Unmap(0, nullptr);
                cameraMappedData_ = nullptr;
            }

            instanceResource_ = CreateBufferResource(
                device_,
                sizeof(ParticleInstanceData) * maximumParticles);
            cameraResource_ = CreateBufferResource(
                device_,
                AlignConstantBufferSize(sizeof(ParticleCameraData)));
            if (!instanceResource_ || !cameraResource_) {
                return false;
            }

            if (FAILED(instanceResource_->Map(
                0, nullptr, &instanceMappedData_))) {
                return false;
            }
            if (FAILED(cameraResource_->Map(
                0, nullptr, &cameraMappedData_))) {
                instanceResource_->Unmap(0, nullptr);
                instanceMappedData_ = nullptr;
                return false;
            }

            instanceBufferView_.BufferLocation =
                instanceResource_->GetGPUVirtualAddress();
            instanceBufferView_.SizeInBytes = static_cast<UINT>(
                sizeof(ParticleInstanceData) * maximumParticles);
            instanceBufferView_.StrideInBytes =
                sizeof(ParticleInstanceData);
            return true;
        }

        bool LoadTexture(
            const std::string& texturePath,
            D3D12_GPU_DESCRIPTOR_HANDLE& textureHandle) {
            if (texturePath.empty()) {
                lastError_ = "Particle texture path is empty";
                return false;
            }
            if (!textureManager_.Load(texturePath, commandList_)) {
                lastError_ = "Failed to load particle texture: " + texturePath;
                return false;
            }
            if (!textureManager_.TryGetGPUDescriptorHandle(
                texturePath, textureHandle)) {
                lastError_ = "Particle texture descriptor is unavailable";
                return false;
            }
            return true;
        }

        void SpawnParticle(const Emitter& emitter) {
            Particle particle;
            particle.emitterId = emitter.id;
            particle.position = emitter.settings.emitter.position;
            particle.position.x += RandomRange(
                -emitter.settings.emitter.spawnArea.x * 0.5f,
                emitter.settings.emitter.spawnArea.x * 0.5f);
            particle.position.y += RandomRange(
                -emitter.settings.emitter.spawnArea.y * 0.5f,
                emitter.settings.emitter.spawnArea.y * 0.5f);
            particle.position.z += RandomRange(
                -emitter.settings.emitter.spawnArea.z * 0.5f,
                emitter.settings.emitter.spawnArea.z * 0.5f);

            particle.velocity = {
                RandomRange(
                    emitter.settings.particle.velocity.min.x,
                    emitter.settings.particle.velocity.max.x),
                RandomRange(
                    emitter.settings.particle.velocity.min.y,
                    emitter.settings.particle.velocity.max.y),
                RandomRange(
                    emitter.settings.particle.velocity.min.z,
                    emitter.settings.particle.velocity.max.z)
            };
            particle.lifeTime = RandomRange(
                emitter.settings.particle.lifeTime.min,
                emitter.settings.particle.lifeTime.max);
            particles_.push_back(particle);
        }

        float RandomRange(float minimum, float maximum) {
            if (minimum > maximum) {
                std::swap(minimum, maximum);
            }
            std::uniform_real_distribution<float> distribution(
                minimum, maximum);
            return distribution(randomEngine_);
        }

        static float Lerp(float start, float end, float amount) {
            return start + (end - start) * amount;
        }

        static Vector4 Lerp(
            const Vector4& start,
            const Vector4& end,
            float amount) {
            return {
                Lerp(start.x, end.x, amount),
                Lerp(start.y, end.y, amount),
                Lerp(start.z, end.z, amount),
                Lerp(start.w, end.w, amount)
            };
        }

        Microsoft::WRL::ComPtr<ID3D12Device> device_;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
        TextureManager textureManager_;
        ParticleEffectData settings_;
        std::vector<Emitter> emitters_;
        std::vector<Particle> particles_;
        std::mt19937 randomEngine_{ std::random_device{}() };

        Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
        Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
        Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource_;
        Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
        D3D12_VERTEX_BUFFER_VIEW instanceBufferView_{};
        D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
        void* instanceMappedData_ = nullptr;
        void* cameraMappedData_ = nullptr;

        EmitterId nextEmitterId_ = 1;
        uint32_t bufferParticleCapacity_ = 0;
        bool isInitialized_ = false;
        bool isPlaying_ = true;
        std::string lastError_;
    };

}
