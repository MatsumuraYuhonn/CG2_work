#pragma once

#include <cstdio>
#include <string>
#include <vector>

#include "MTEngine/Engine/Particle/ParticleEffectSerializer.h"
#include "MTEngine/Engine/Particle/ParticleSystem.h"
#include "MTEngine/Engine/Debug/DebugCamera.h"
#include "MTEngine/Engine/Math/Matrix.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "MTEngine/Engine/externals/ImGuizmo/ImGuizmo.h"
#endif

namespace MTEngine {

    class ParticleEditor {
    public:
        void Initialize(
            ParticleSystem* particleSystem,
            const std::string& filePath,
            const ParticleEffectData& data) {

            particleSystem_ = particleSystem;
            filePath_ = filePath;
            editingData_ = data;
            CopyText(filePathBuffer_, sizeof(filePathBuffer_), filePath_);
            CopyText(
                texturePathBuffer_,
                sizeof(texturePathBuffer_),
                editingData_.render.texturePath);
            if (particleSystem_) {
                const std::vector<ParticleSystem::EmitterId> emitterIds =
                    particleSystem_->GetEmitterIds();
                if (!emitterIds.empty()) {
                    selectedEmitterId_ = emitterIds.front();
                }
            }
            statusMessage_ = "Particle editor ready";
            statusIsError_ = false;
        }

        void Draw() {
#ifdef USE_IMGUI
            ImGui::Text("Particle Effect");
            ImGui::Separator();

            ImGui::InputText(
                "JSON File",
                filePathBuffer_,
                sizeof(filePathBuffer_));

            if (ImGui::Button("Load JSON")) {
                Load();
            }
            ImGui::SameLine();
            if (ImGui::Button("Save JSON")) {
                Save();
            }
            ImGui::SameLine();
            if (ImGui::Button("Apply")) {
                Apply();
            }

            if (particleSystem_) {
                char selectedEmitterLabel[64]{};
                if (selectedEmitterId_ != 0) {
                    std::snprintf(
                        selectedEmitterLabel,
                        sizeof(selectedEmitterLabel),
                        "Emitter %llu",
                        static_cast<unsigned long long>(selectedEmitterId_));
                } else {
                    CopyText(
                        selectedEmitterLabel,
                        sizeof(selectedEmitterLabel),
                        "No emitter");
                }

                if (ImGui::BeginCombo(
                    "Emitter Instance",
                    selectedEmitterLabel)) {
                    const std::vector<ParticleSystem::EmitterId> emitterIds =
                        particleSystem_->GetEmitterIds();
                    for (ParticleSystem::EmitterId emitterId : emitterIds) {
                        char emitterLabel[64]{};
                        std::snprintf(
                            emitterLabel,
                            sizeof(emitterLabel),
                            "Emitter %llu",
                            static_cast<unsigned long long>(emitterId));
                        const bool isSelected =
                            emitterId == selectedEmitterId_;
                        if (ImGui::Selectable(emitterLabel, isSelected)) {
                            SelectEmitter(emitterId);
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::Button("Add Emitter")) {
                    ParticleEffectData emitterData;
                    const ParticleSystem::EmitterId emitterId =
                        particleSystem_->CreateEmitter(emitterData);
                    if (emitterId == 0) {
                        SetError(particleSystem_->GetLastError());
                    } else {
                        SelectEmitter(emitterId);
                        statusMessage_ = "Emitter added";
                        statusIsError_ = false;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove Emitter") &&
                    selectedEmitterId_ != 0) {
                    const ParticleSystem::EmitterId removedEmitterId =
                        selectedEmitterId_;
                    if (particleSystem_->RemoveEmitter(removedEmitterId)) {
                        selectedEmitterId_ = 0;
                        const std::vector<ParticleSystem::EmitterId> emitterIds =
                            particleSystem_->GetEmitterIds();
                        if (!emitterIds.empty()) {
                            SelectEmitter(emitterIds.front());
                        }
                        statusMessage_ = "Emitter removed";
                        statusIsError_ = false;
                    }
                }

                if (selectedEmitterId_ == 0) {
                    ImGui::Text("Create an emitter to edit its settings.");
                }
            }

            if (particleSystem_ && selectedEmitterId_ != 0) {
                const char* playButtonLabel = "Pause";
                if (!particleSystem_->IsPlaying(selectedEmitterId_)) {
                    playButtonLabel = "Play";
                }
                if (ImGui::Button(playButtonLabel)) {
                    particleSystem_->SetPlaying(
                        selectedEmitterId_,
                        !particleSystem_->IsPlaying(selectedEmitterId_));
                }
                ImGui::SameLine();
                if (ImGui::Button("Restart")) {
                    particleSystem_->Restart(selectedEmitterId_);
                }
                ImGui::SameLine();
                ImGui::Text(
                    "Alive: %zu / %u",
                    particleSystem_->GetActiveParticleCount(
                        selectedEmitterId_),
                    editingData_.emitter.maxParticles);
            }

            if (statusIsError_) {
                ImGui::TextColored(
                    ImVec4(1.0f, 0.35f, 0.3f, 1.0f),
                    "%s",
                    statusMessage_.c_str());
            } else {
                ImGui::TextColored(
                    ImVec4(0.38f, 0.78f, 0.52f, 1.0f),
                    "%s",
                    statusMessage_.c_str());
            }

            bool changed = false;

            if (ImGui::CollapsingHeader(
                "Emitter",
                ImGuiTreeNodeFlags_DefaultOpen)) {
                changed |= ImGui::DragFloat3(
                    "Position",
                    &editingData_.emitter.position.x,
                    0.1f);
                changed |= ImGui::DragFloat3(
                    "Spawn Area",
                    &editingData_.emitter.spawnArea.x,
                    0.05f,
                    0.0f,
                    1000.0f);
                changed |= ImGui::DragFloat(
                    "Spawn Rate",
                    &editingData_.emitter.spawnRate,
                    1.0f,
                    0.0f,
                    10000.0f,
                    "%.1f / sec");
                changed |= ImGui::DragFloat(
                    "Emission Duration",
                    &editingData_.emitter.emissionDuration,
                    0.05f,
                    0.01f,
                    3600.0f,
                    "%.2f sec");

                int maximumParticles = static_cast<int>(
                    editingData_.emitter.maxParticles);
                if (ImGui::DragInt(
                    "Max Particles",
                    &maximumParticles,
                    1.0f,
                    1,
                    static_cast<int>(
                        ParticleEffectData::kMaximumParticleLimit))) {
                    editingData_.emitter.maxParticles =
                        static_cast<uint32_t>(maximumParticles);
                    changed = true;
                }
                changed |= ImGui::Checkbox(
                    "Loop",
                    &editingData_.emitter.loop);
            }

            if (ImGui::CollapsingHeader(
                "Particle",
                ImGuiTreeNodeFlags_DefaultOpen)) {
                changed |= ImGui::DragFloat(
                    "Life Time Min",
                    &editingData_.particle.lifeTime.min,
                    0.01f,
                    0.01f,
                    60.0f);
                changed |= ImGui::DragFloat(
                    "Life Time Max",
                    &editingData_.particle.lifeTime.max,
                    0.01f,
                    0.01f,
                    60.0f);
                changed |= ImGui::DragFloat3(
                    "Velocity Min",
                    &editingData_.particle.velocity.min.x,
                    0.05f);
                changed |= ImGui::DragFloat3(
                    "Velocity Max",
                    &editingData_.particle.velocity.max.x,
                    0.05f);
                changed |= ImGui::DragFloat3(
                    "Acceleration",
                    &editingData_.particle.acceleration.x,
                    0.05f);
                changed |= ImGui::ColorEdit4(
                    "Start Color",
                    &editingData_.particle.startColor.x);
                changed |= ImGui::ColorEdit4(
                    "End Color",
                    &editingData_.particle.endColor.x);
                changed |= ImGui::DragFloat(
                    "Start Size",
                    &editingData_.particle.startSize,
                    0.01f,
                    0.0f,
                    100.0f);
                changed |= ImGui::DragFloat(
                    "End Size",
                    &editingData_.particle.endSize,
                    0.01f,
                    0.0f,
                    100.0f);
            }

            if (ImGui::CollapsingHeader(
                "Render",
                ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::InputText(
                    "Texture",
                    texturePathBuffer_,
                    sizeof(texturePathBuffer_));
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    editingData_.render.texturePath = texturePathBuffer_;
                    changed = true;
                }

                const char* blendModes[] = { "Alpha", "Additive" };
                int selectedBlendMode = 1;
                if (editingData_.render.blendMode ==
                    ParticleBlendMode::Alpha) {
                    selectedBlendMode = 0;
                }
                if (ImGui::Combo(
                    "Blend Mode",
                    &selectedBlendMode,
                    blendModes,
                    2)) {
                    editingData_.render.blendMode =
                        ParticleBlendMode::Additive;
                    if (selectedBlendMode == 0) {
                        editingData_.render.blendMode =
                            ParticleBlendMode::Alpha;
                    }
                    changed = true;
                }
                changed |= ImGui::Checkbox(
                    "Billboard",
                    &editingData_.render.billboard);
            }

            if (changed) {
                Apply();
            }
#endif
        }

        void DrawSceneVisualization(const DebugCamera& camera) const {
#ifdef USE_IMGUI
            if (!particleSystem_ || selectedEmitterId_ == 0) {
                return;
            }

            const ParticleEffectData* emitterData =
                particleSystem_->GetEmitterData(selectedEmitterId_);
            if (!emitterData) {
                return;
            }

            Vector3 visualizationSize = emitterData->emitter.spawnArea;
            constexpr float kMinimumVisualizationSize = 0.01f;
            if (visualizationSize.x < kMinimumVisualizationSize) {
                visualizationSize.x = kMinimumVisualizationSize;
            }
            if (visualizationSize.y < kMinimumVisualizationSize) {
                visualizationSize.y = kMinimumVisualizationSize;
            }
            if (visualizationSize.z < kMinimumVisualizationSize) {
                visualizationSize.z = kMinimumVisualizationSize;
            }

            const Matrix4x4 spawnAreaWorld = MakeAffineMatrix(
                visualizationSize,
                {},
                emitterData->emitter.position);
            ImGuizmo::DrawCubes(
                &camera.GetViewMatrix().m[0][0],
                &camera.GetProjectionMatrix().m[0][0],
                &spawnAreaWorld.m[0][0],
                1,
                true);
#else
            (void)camera;
#endif
        }

        const ParticleEffectData& GetEditingData() const {
            return editingData_;
        }

        const std::string& GetFilePath() const {
            return filePath_;
        }

        bool SaveCurrentData() {
            return ParticleEffectSerializer::Save(
                filePath_, editingData_);
        }

    private:
        void Apply() {
            editingData_.render.texturePath = texturePathBuffer_;
            ValidateParticleEffectData(editingData_);

            if (!particleSystem_) {
                SetError("ParticleSystem is not available");
                return;
            }
            if (selectedEmitterId_ == 0) {
                SetError("Particle emitter is not selected");
                return;
            }
            if (!particleSystem_->ApplySettings(
                selectedEmitterId_,
                editingData_)) {
                SetError(particleSystem_->GetLastError());
                return;
            }
            statusMessage_ = "Applied to runtime";
            statusIsError_ = false;
        }

        void Load() {
            filePath_ = filePathBuffer_;
            ParticleEffectData loaded;
            if (!ParticleEffectSerializer::Load(filePath_, loaded)) {
                SetError("Failed to load JSON: " + filePath_);
                return;
            }

            editingData_ = loaded;
            CopyText(
                texturePathBuffer_,
                sizeof(texturePathBuffer_),
                editingData_.render.texturePath);
            Apply();
            if (!statusIsError_) {
                particleSystem_->Restart(selectedEmitterId_);
                statusMessage_ = "Loaded: " + filePath_;
            }
        }

        void Save() {
            filePath_ = filePathBuffer_;
            editingData_.render.texturePath = texturePathBuffer_;
            ValidateParticleEffectData(editingData_);
            if (!ParticleEffectSerializer::Save(filePath_, editingData_)) {
                SetError("Failed to save JSON: " + filePath_);
                return;
            }
            statusMessage_ = "Saved: " + filePath_;
            statusIsError_ = false;
        }

        void SetError(const std::string& message) {
            statusMessage_ = message;
            statusIsError_ = true;
        }

        void SelectEmitter(ParticleSystem::EmitterId emitterId) {
            if (!particleSystem_) {
                return;
            }
            const ParticleEffectData* emitterData =
                particleSystem_->GetEmitterData(emitterId);
            if (!emitterData) {
                return;
            }
            selectedEmitterId_ = emitterId;
            editingData_ = *emitterData;
            CopyText(
                texturePathBuffer_,
                sizeof(texturePathBuffer_),
                editingData_.render.texturePath);
        }

        static void CopyText(
            char* destination,
            size_t destinationSize,
            const std::string& source) {
            std::snprintf(
                destination,
                destinationSize,
                "%s",
                source.c_str());
        }

        ParticleSystem* particleSystem_ = nullptr;
        ParticleSystem::EmitterId selectedEmitterId_ = 0;
        ParticleEffectData editingData_;
        std::string filePath_;
        std::string statusMessage_;
        bool statusIsError_ = false;
        char filePathBuffer_[512]{};
        char texturePathBuffer_[512]{};
    };

}
