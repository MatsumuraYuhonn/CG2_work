#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>

#include "MTEngine/Engine/Particle/ParticleEffectData.h"
#include "MTEngine/Engine/externals/json.hpp"

namespace MTEngine {

    class ParticleEffectSerializer {
    public:
        static bool Save(
            const std::string& filePath,
            const ParticleEffectData& source) {

            if (filePath.empty()) {
                return false;
            }

            ParticleEffectData data = source;
            ValidateParticleEffectData(data);
            
            nlohmann::json root;
            root["version"] = data.version;
            root["name"] = data.name;
            root["emitter"] = {
                {"position", ToJson(data.emitter.position)},
                {"spawnArea", ToJson(data.emitter.spawnArea)},
                {"spawnRate", data.emitter.spawnRate},
                {"emissionDuration", data.emitter.emissionDuration},
                {"maxParticles", data.emitter.maxParticles},
                {"loop", data.emitter.loop}
            };
            root["particle"] = {
                {"lifeTime", {
                    {"min", data.particle.lifeTime.min},
                    {"max", data.particle.lifeTime.max}
                }},
                {"velocity", {
                    {"min", ToJson(data.particle.velocity.min)},
                    {"max", ToJson(data.particle.velocity.max)}
                }},
                {"acceleration", ToJson(data.particle.acceleration)},
                {"startColor", ToJson(data.particle.startColor)},
                {"endColor", ToJson(data.particle.endColor)},
                {"startSize", data.particle.startSize},
                {"endSize", data.particle.endSize}
            };
            root["render"] = {
                {"texture", data.render.texturePath},
                {"blendMode", BlendModeToString(data.render.blendMode)},
                {"billboard", data.render.billboard}
            };

            const std::filesystem::path path(filePath);
            std::error_code error;
            if (!path.parent_path().empty()) {
                std::filesystem::create_directories(path.parent_path(), error);
                if (error) {
                    return false;
                }
            }

            std::ofstream file(path);
            if (!file.is_open()) {
                return false;
            }

            file << root.dump(4);
            return file.good();
        }

        static bool Load(
            const std::string& filePath,
            ParticleEffectData& destination) {

            std::ifstream file(filePath);
            if (!file.is_open()) {
                return false;
            }

            try {
                nlohmann::json root;
                file >> root;

                ParticleEffectData loaded;
                loaded.version = root.at("version").get<int>();
                if (loaded.version != ParticleEffectData::kCurrentVersion) {
                    return false;
                }

                loaded.name = root.at("name").get<std::string>();

                const nlohmann::json& emitter = root.at("emitter");
                loaded.emitter.position =
                    ReadVector3(emitter.at("position"));
                loaded.emitter.spawnArea =
                    ReadVector3(emitter.at("spawnArea"));
                loaded.emitter.spawnRate =
                    emitter.at("spawnRate").get<float>();
                loaded.emitter.emissionDuration =
                    emitter.at("emissionDuration").get<float>();
                loaded.emitter.maxParticles =
                    emitter.at("maxParticles").get<uint32_t>();
                loaded.emitter.loop =
                    emitter.at("loop").get<bool>();

                const nlohmann::json& particle = root.at("particle");
                const nlohmann::json& lifeTime = particle.at("lifeTime");
                loaded.particle.lifeTime.min =
                    lifeTime.at("min").get<float>();
                loaded.particle.lifeTime.max =
                    lifeTime.at("max").get<float>();

                const nlohmann::json& velocity = particle.at("velocity");
                loaded.particle.velocity.min =
                    ReadVector3(velocity.at("min"));
                loaded.particle.velocity.max =
                    ReadVector3(velocity.at("max"));
                loaded.particle.acceleration =
                    ReadVector3(particle.at("acceleration"));
                loaded.particle.startColor =
                    ReadVector4(particle.at("startColor"));
                loaded.particle.endColor =
                    ReadVector4(particle.at("endColor"));
                loaded.particle.startSize =
                    particle.at("startSize").get<float>();
                loaded.particle.endSize =
                    particle.at("endSize").get<float>();

                const nlohmann::json& render = root.at("render");
                loaded.render.texturePath =
                    render.at("texture").get<std::string>();
                loaded.render.blendMode = StringToBlendMode(
                    render.at("blendMode").get<std::string>());
                loaded.render.billboard =
                    render.at("billboard").get<bool>();

                ValidateParticleEffectData(loaded);
                destination = std::move(loaded);
                return true;
            }
            catch (const nlohmann::json::exception&) {
                return false;
            }
        }

    private:
        static nlohmann::json ToJson(const Vector3& value) {
            return { value.x, value.y, value.z };
        }

        static nlohmann::json ToJson(const Vector4& value) {
            return { value.x, value.y, value.z, value.w };
        }

        static Vector3 ReadVector3(const nlohmann::json& value) {
            return {
                value.at(0).get<float>(),
                value.at(1).get<float>(),
                value.at(2).get<float>()
            };
        }

        static Vector4 ReadVector4(const nlohmann::json& value) {
            return {
                value.at(0).get<float>(),
                value.at(1).get<float>(),
                value.at(2).get<float>(),
                value.at(3).get<float>()
            };
        }

        static const char* BlendModeToString(ParticleBlendMode blendMode) {
            if (blendMode == ParticleBlendMode::Alpha) {
                return "Alpha";
            }
            return "Additive";
        }

        static ParticleBlendMode StringToBlendMode(
            const std::string& blendMode) {
            if (blendMode == "Alpha") {
                return ParticleBlendMode::Alpha;
            }
            return ParticleBlendMode::Additive;
        }
    };

}
