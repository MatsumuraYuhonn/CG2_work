#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
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
            if (!LoadTexture(settings_.render.texturePath)) {
                return false;
            }

            particles_.reserve(settings_.emitter.maxParticles);
            isInitialized_ = true;
            lastError_.clear();
            return true;
        }

        bool ApplySettings(const ParticleEffectData& source) {
            if (!isInitialized_) {
                return false;
            }

            ParticleEffectData settings = source;
            ValidateParticleEffectData(settings);

            if (settings.render.texturePath != settings_.render.texturePath) {
                if (!LoadTexture(settings.render.texturePath)) {
                    return false;
                }
            }

            if (settings.emitter.maxParticles !=
                settings_.emitter.maxParticles) {
                if (!CreateInstanceBuffers(
                    settings.emitter.maxParticles)) {
                    lastError_ = "Failed to resize particle instance buffers";
                    return false;
                }

                if (particles_.size() > settings.emitter.maxParticles) {
                    particles_.resize(settings.emitter.maxParticles);
                }
                particles_.reserve(settings.emitter.maxParticles);
            }

            settings_ = settings;
            lastError_.clear();
            return true;
        }

        void Update(float deltaTime) {
            if (!isInitialized_ || !isPlaying_) {
                return;
            }

            deltaTime = (std::clamp)(deltaTime, 0.0f, 0.1f);
            emissionElapsed_ += deltaTime;

            bool canEmit = settings_.emitter.loop;
            if (!settings_.emitter.loop) {
                canEmit =
                    emissionElapsed_ <= settings_.emitter.emissionDuration;
            }

            if (canEmit && settings_.emitter.spawnRate > 0.0f) {
                spawnAccumulator_ +=
                    settings_.emitter.spawnRate * deltaTime;

                const uint32_t spawnCount =
                    static_cast<uint32_t>(spawnAccumulator_);
                spawnAccumulator_ -= static_cast<float>(spawnCount);

                for (uint32_t index = 0; index < spawnCount; ++index) {
                    if (particles_.size() >=
                        settings_.emitter.maxParticles) {
                        break;
                    }
                    SpawnParticle();
                }
            }

            for (Particle& particle : particles_) {
                particle.age += deltaTime;
                particle.velocity.x +=
                    settings_.particle.acceleration.x * deltaTime;
                particle.velocity.y +=
                    settings_.particle.acceleration.y * deltaTime;
                particle.velocity.z +=
                    settings_.particle.acceleration.z * deltaTime;
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

            BlendMode blendMode = BlendMode::kBlendModeAdd;
            if (settings_.render.blendMode == ParticleBlendMode::Alpha) {
                blendMode = BlendMode::kBlendModeNormal;
            }
            renderer->SetParticlePipelineState(
                blendMode,
                DepthWriteMode::kDisable);

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
            for (size_t index = 0; index < particles_.size(); ++index) {
                const Particle& particle = particles_[index];
                float normalizedAge = particle.age / particle.lifeTime;
                normalizedAge = (std::clamp)(normalizedAge, 0.0f, 1.0f);

                const float size = Lerp(
                    settings_.particle.startSize,
                    settings_.particle.endSize,
                    normalizedAge);
                const Vector4 color = Lerp(
                    settings_.particle.startColor,
                    settings_.particle.endColor,
                    normalizedAge);

                Matrix4x4 world = MakeScaleMatrix({ size, size, size });
                if (settings_.render.billboard) {
                    world = Multiply(world, billboardMatrix);
                }
                world = Multiply(
                    world,
                    MakeTranslateMatrix(particle.position));

                instanceData[index].world = world;
                instanceData[index].color = color;
            }

            ParticleCameraData* cameraData =
                static_cast<ParticleCameraData*>(cameraMappedData_);
            cameraData->viewProjection = viewProjection;

            commandList->SetGraphicsRootConstantBufferView(
                0,
                cameraResource_->GetGPUVirtualAddress());
            commandList->SetGraphicsRootDescriptorTable(
                1,
                textureHandle_);
            commandList->DrawIndexedInstanced(
                6,
                static_cast<UINT>(particles_.size()),
                0,
                0,
                0);

            renderer->SetPipelineState(
                BlendMode::kBlendModeNormal,
                DepthWriteMode::kEnable);
        }

        void Restart() {
            particles_.clear();
            spawnAccumulator_ = 0.0f;
            emissionElapsed_ = 0.0f;
            isPlaying_ = true;
        }

        void SetPlaying(bool isPlaying) {
            isPlaying_ = isPlaying;
        }

        bool IsPlaying() const {
            return isPlaying_;
        }

        size_t GetActiveParticleCount() const {
            return particles_.size();
        }

        const ParticleEffectData& GetSettings() const {
            return settings_;
        }

        const std::string& GetLastError() const {
            return lastError_;
        }

    private:
        struct Particle {
            Vector3 position{};
            Vector3 velocity{};
            float age = 0.0f;
            float lifeTime = 1.0f;
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

        bool LoadTexture(const std::string& texturePath) {
            if (texturePath.empty()) {
                lastError_ = "Particle texture path is empty";
                return false;
            }
            if (!textureManager_.Load(texturePath, commandList_)) {
                lastError_ = "Failed to load particle texture: " + texturePath;
                return false;
            }
            if (!textureManager_.TryGetGPUDescriptorHandle(
                texturePath, textureHandle_)) {
                lastError_ = "Particle texture descriptor is unavailable";
                return false;
            }
            return true;
        }

        void SpawnParticle() {
            Particle particle;
            particle.position = settings_.emitter.position;
            particle.position.x += RandomRange(
                -settings_.emitter.spawnArea.x * 0.5f,
                settings_.emitter.spawnArea.x * 0.5f);
            particle.position.y += RandomRange(
                -settings_.emitter.spawnArea.y * 0.5f,
                settings_.emitter.spawnArea.y * 0.5f);
            particle.position.z += RandomRange(
                -settings_.emitter.spawnArea.z * 0.5f,
                settings_.emitter.spawnArea.z * 0.5f);

            particle.velocity = {
                RandomRange(
                    settings_.particle.velocity.min.x,
                    settings_.particle.velocity.max.x),
                RandomRange(
                    settings_.particle.velocity.min.y,
                    settings_.particle.velocity.max.y),
                RandomRange(
                    settings_.particle.velocity.min.z,
                    settings_.particle.velocity.max.z)
            };
            particle.lifeTime = RandomRange(
                settings_.particle.lifeTime.min,
                settings_.particle.lifeTime.max);
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
        std::vector<Particle> particles_;
        std::mt19937 randomEngine_{ std::random_device{}() };

        Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
        Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
        Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource_;
        Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
        D3D12_VERTEX_BUFFER_VIEW instanceBufferView_{};
        D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};
        void* instanceMappedData_ = nullptr;
        void* cameraMappedData_ = nullptr;

        float spawnAccumulator_ = 0.0f;
        float emissionElapsed_ = 0.0f;
        bool isInitialized_ = false;
        bool isPlaying_ = true;
        std::string lastError_;
    };

}
