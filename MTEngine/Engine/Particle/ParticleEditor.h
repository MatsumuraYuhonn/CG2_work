#pragma once

#include <cstdio>
#include <string>

#include "MTEngine/Engine/Particle/ParticleEffectSerializer.h"
#include "MTEngine/Engine/Particle/ParticleSystem.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
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
            CopyText(nameBuffer_, sizeof(nameBuffer_), editingData_.name);
            CopyText(
                texturePathBuffer_,
                sizeof(texturePathBuffer_),
                editingData_.render.texturePath);
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
                const char* playButtonLabel = "Pause";
                if (!particleSystem_->IsPlaying()) {
                    playButtonLabel = "Play";
                }
                if (ImGui::Button(playButtonLabel)) {
                    particleSystem_->SetPlaying(
                        !particleSystem_->IsPlaying());
                }
                ImGui::SameLine();
                if (ImGui::Button("Restart")) {
                    particleSystem_->Restart();
                }
                ImGui::SameLine();
                ImGui::Text(
                    "Alive: %zu / %u",
                    particleSystem_->GetActiveParticleCount(),
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
                "Effect",
                ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::InputText(
                    "Name",
                    nameBuffer_,
                    sizeof(nameBuffer_))) {
                    editingData_.name = nameBuffer_;
                    changed = true;
                }
            }

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
            if (!particleSystem_->ApplySettings(editingData_)) {
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
            CopyText(nameBuffer_, sizeof(nameBuffer_), editingData_.name);
            CopyText(
                texturePathBuffer_,
                sizeof(texturePathBuffer_),
                editingData_.render.texturePath);
            Apply();
            if (!statusIsError_) {
                particleSystem_->Restart();
                statusMessage_ = "Loaded: " + filePath_;
            }
        }

        void Save() {
            filePath_ = filePathBuffer_;
            editingData_.name = nameBuffer_;
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
        ParticleEffectData editingData_;
        std::string filePath_;
        std::string statusMessage_;
        bool statusIsError_ = false;
        char filePathBuffer_[512]{};
        char nameBuffer_[128]{};
        char texturePathBuffer_[512]{};
    };

}
