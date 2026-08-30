#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace MTEngine {

    class ClearTimeRecorder {
    public:
        static bool Save(float clearTimeSeconds, bool isNightmareMode) {
            if (!std::isfinite(clearTimeSeconds) || clearTimeSeconds <= 0.0f) {
                return false;
            }

            ClearTimeData data = Load();
            ModeData& targetMode = isNightmareMode
                ? data.nightmare
                : data.normal;
            ++targetMode.clearCount;
            targetMode.allTimes.push_back(
                static_cast<double>(clearTimeSeconds));
            return Write(data);
        }

        static std::vector<double> GetBestTimes(bool isNightmareMode) {
            ClearTimeData data = Load();
            std::vector<double> bestTimes = isNightmareMode
                ? data.nightmare.allTimes
                : data.normal.allTimes;
            NormalizeBestTimes(bestTimes);
            return bestTimes;
        }

        static std::filesystem::path GetSaveFilePath() {
            return std::filesystem::path("MTEngine") /
                "SaveData" / "clear_times.json";
        }

    private:
        struct ModeData {
            std::vector<double> allTimes;
            uint64_t clearCount = 0;
        };

        struct ClearTimeData {
            ModeData normal;
            ModeData nightmare;
        };

        static std::string ExtractModeObject(
            const std::string& json,
            std::string_view modeName) {
            const std::string modeKey = "\"" + std::string(modeName) + "\"";
            const size_t modePosition = json.find(modeKey);
            if (modePosition == std::string::npos) {
                return {};
            }
            const size_t objectStart = json.find('{', modePosition);
            const size_t objectEnd = json.find('}', objectStart);
            if (objectStart == std::string::npos ||
                objectEnd == std::string::npos) {
                return {};
            }
            return json.substr(objectStart, objectEnd - objectStart + 1);
        }

        static std::vector<double> ExtractTimes(const std::string& modeJson) {
            size_t timesPosition = modeJson.find("\"all_times_seconds\"");
            if (timesPosition == std::string::npos) {
                // Version 2 retained only the top three. Preserve those
                // records if such a save already exists.
                timesPosition = modeJson.find("\"best_times_seconds\"");
            }
            if (timesPosition == std::string::npos) {
                // Version 1 used times_seconds for the complete history.
                timesPosition = modeJson.find("\"times_seconds\"");
            }
            const size_t arrayStart = modeJson.find('[', timesPosition);
            const size_t arrayEnd = modeJson.find(']', arrayStart);
            if (timesPosition == std::string::npos ||
                arrayStart == std::string::npos ||
                arrayEnd == std::string::npos) {
                return {};
            }

            const std::string arrayText = modeJson.substr(
                arrayStart + 1,
                arrayEnd - arrayStart - 1);
            std::vector<double> times;
            const char* current = arrayText.c_str();
            const char* const end = current + arrayText.size();
            while (current < end) {
                char* next = nullptr;
                const double value = std::strtod(current, &next);
                if (next == current) {
                    ++current;
                    continue;
                }
                if (std::isfinite(value) && value > 0.0) {
                    times.push_back(value);
                }
                current = next;
            }
            return times;
        }

        static uint64_t ExtractClearCount(const std::string& modeJson) {
            const size_t countPosition = modeJson.find("\"clear_count\"");
            const size_t valuePosition = modeJson.find(':', countPosition);
            if (countPosition == std::string::npos ||
                valuePosition == std::string::npos) {
                return 0;
            }
            const char* valueStart = modeJson.c_str() + valuePosition + 1;
            char* valueEnd = nullptr;
            return std::strtoull(valueStart, &valueEnd, 10);
        }

        static void NormalizeBestTimes(std::vector<double>& times) {
            std::sort(times.begin(), times.end());
            constexpr size_t kMaximumBestTimeCount = 3;
            if (times.size() > kMaximumBestTimeCount) {
                times.resize(kMaximumBestTimeCount);
            }
        }

        static ModeData LoadMode(
            const std::string& json,
            std::string_view modeName) {
            const std::string modeJson = ExtractModeObject(json, modeName);
            ModeData modeData;
            modeData.allTimes = ExtractTimes(modeJson);
            modeData.clearCount = ExtractClearCount(modeJson);
            if (modeData.clearCount < modeData.allTimes.size()) {
                modeData.clearCount = modeData.allTimes.size();
            }
            return modeData;
        }

        static ClearTimeData Load() {
            std::ifstream input(GetSaveFilePath(), std::ios::binary);
            if (!input) {
                return {};
            }
            const std::string json{
                std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>() };
            return {
                LoadMode(json, "normal"),
                LoadMode(json, "nightmare"),
            };
        }

        static void WriteMode(
            std::ofstream& output,
            std::string_view modeName,
            const ModeData& modeData,
            bool writeTrailingComma) {
            output << "  \"" << modeName << "\": {\n";
            output << "    \"clear_count\": " << modeData.clearCount << ",\n"
                << "    \"all_times_seconds\": [";
            for (size_t index = 0; index < modeData.allTimes.size(); ++index) {
                if (index > 0) {
                    output << ", ";
                }
                output << modeData.allTimes[index];
            }
            output << "]\n  }" << (writeTrailingComma ? "," : "") << "\n";
        }

        static bool Write(const ClearTimeData& data) {
            const std::filesystem::path savePath = GetSaveFilePath();
            std::error_code error;
            std::filesystem::create_directories(savePath.parent_path(), error);
            if (error) {
                return false;
            }

            const std::filesystem::path temporaryPath = savePath.string() + ".tmp";
            std::ofstream output(
                temporaryPath,
                std::ios::binary | std::ios::trunc);
            if (!output) {
                return false;
            }
            output << std::fixed << std::setprecision(3)
                << "{\n  \"version\": 3,\n";
            WriteMode(output, "normal", data.normal, true);
            WriteMode(output, "nightmare", data.nightmare, false);
            output << "}\n";
            output.flush();
            if (!output) {
                return false;
            }
            output.close();

            if (std::filesystem::exists(savePath, error)) {
                error.clear();
                std::filesystem::copy_file(
                    temporaryPath,
                    savePath,
                    std::filesystem::copy_options::overwrite_existing,
                    error);
                if (error) {
                    return false;
                }
                std::filesystem::remove(temporaryPath, error);
                return true;
            }

            error.clear();
            std::filesystem::rename(temporaryPath, savePath, error);
            return !error;
        }
    };

}
