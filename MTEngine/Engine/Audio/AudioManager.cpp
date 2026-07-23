// AudioManager.cpp
#include "AudioManager.h"
#include <cassert>

namespace MTEngine {

    AudioManager::~AudioManager() {
        Finalize();
    }

    void AudioManager::Initialize() {
        HRESULT hr = XAudio2Create(xAudio2_.GetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR);
        assert(SUCCEEDED(hr));

        hr = xAudio2_->CreateMasteringVoice(&masterVoice_);
        assert(SUCCEEDED(hr));
    }

    void AudioManager::Finalize() {
        UnloadAll();

        if (masterVoice_) {
            masterVoice_->DestroyVoice();
            masterVoice_ = nullptr;
        }

        xAudio2_.Reset();
    }

    void AudioManager::LoadWave(const std::string& filePath) {
        // すでにロード済みならスキップ
        if (soundDataMap_.find(filePath) != soundDataMap_.end()) {
            return;
        }

        // 読み込んでマップに格納
        SoundData soundData = SoundLoadWave(filePath.c_str());
        soundDataMap_[filePath] = soundData;
    }

    std::unique_ptr<Audio> AudioManager::Play(const std::string& filePath, bool loop, float volume) {
        // 未読み込みの場合は自動でロード
        if (soundDataMap_.find(filePath) == soundDataMap_.end()) {
            LoadWave(filePath);
        }

        auto audio = std::make_unique<Audio>();
        audio->Initialize(xAudio2_.Get(), soundDataMap_[filePath], loop);
        audio->SetVolume(volume);
        audio->Play();

        return audio;
    }

    void AudioManager::Unload(const std::string& filePath) {
        auto it = soundDataMap_.find(filePath);
        if (it != soundDataMap_.end()) {
            SoundUnload(&it->second);
            soundDataMap_.erase(it);
        }
    }

    void AudioManager::UnloadAll() {
        for (auto& [path, soundData] : soundDataMap_) {
            SoundUnload(&soundData);
        }
        soundDataMap_.clear();
    }

}