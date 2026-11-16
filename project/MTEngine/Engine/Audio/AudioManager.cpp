// AudioManager.cpp
#include "AudioManager.h"
#include <cassert>
#include <mfapi.h>

namespace MTEngine {

    AudioManager::~AudioManager() {
        Finalize();
    }

    void AudioManager::Initialize() {
        HRESULT hr = MFStartup(MF_VERSION);
        assert(SUCCEEDED(hr));
        isMediaFoundationInitialized_ = SUCCEEDED(hr);

        hr = XAudio2Create(xAudio2_.GetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR);
        assert(SUCCEEDED(hr));

        // Use the normal Windows audio endpoint instead of XAudio2's virtual
        // endpoint so the game follows the PC master volume and per-app mixer.
        hr = xAudio2_->CreateMasteringVoice(
            &masterVoice_,
            XAUDIO2_DEFAULT_CHANNELS,
            XAUDIO2_DEFAULT_SAMPLERATE,
            XAUDIO2_NO_VIRTUAL_AUDIO_CLIENT,
            nullptr,
            nullptr,
            AudioCategory_GameEffects);
        assert(SUCCEEDED(hr));
    }

    void AudioManager::Finalize() {
        UnloadAll();

        if (masterVoice_) {
            masterVoice_->DestroyVoice();
            masterVoice_ = nullptr;
        }

        xAudio2_.Reset();

        if (isMediaFoundationInitialized_) {
            MFShutdown();
            isMediaFoundationInitialized_ = false;
        }
    }

    void AudioManager::SetMasterVolume(float volume) {
        if (!masterVoice_) {
            return;
        }
        volume = volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume);
        const HRESULT hr = masterVoice_->SetVolume(volume);
        assert(SUCCEEDED(hr));
    }

    void AudioManager::LoadWave(const std::string& filePath) {
        // すでにロード済みならスキップ
        if (soundDataMap_.find(filePath) != soundDataMap_.end()) {
            return;
        }

        // 読み込んでマップに格納
        SoundData soundData = SoundLoad(filePath.c_str());
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
