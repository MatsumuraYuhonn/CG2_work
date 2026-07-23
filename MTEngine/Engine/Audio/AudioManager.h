#pragma once
#include <xaudio2.h>
#include <wrl.h>
#include <string>
#include <unordered_map>
#include <memory>
#include "Sound.h"
#include "Audio.h"

namespace MTEngine {

    class AudioManager {
    public:
        AudioManager() = default;
        ~AudioManager();

        void Initialize();
        void Finalize();

        // Wavファイルのロード（読み込み済みなら既存データを返す）
        void LoadWave(const std::string& filePath);

        // サウンドの再生（Audioインスタンスを生成して返す）
        std::unique_ptr<Audio> Play(const std::string& filePath, bool loop = false, float volume = 1.0f);

        // ロード済みデータの解放
        void Unload(const std::string& filePath);
        void UnloadAll();

        IXAudio2* GetXAudio2() const { return xAudio2_.Get(); }

    private:
        Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
        IXAudio2MasteringVoice* masterVoice_ = nullptr;

        // パスをキーにして読み込んだSoundDataを保持
        std::unordered_map<std::string, SoundData> soundDataMap_;
    };

}