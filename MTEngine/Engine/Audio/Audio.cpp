
#include "Audio.h"
#include <cassert>

namespace MTEngine {

    Audio::~Audio() {
        if (pSourceVoice_) {
            pSourceVoice_->Stop();
            pSourceVoice_->DestroyVoice();
            pSourceVoice_ = nullptr;
        }
    }

    bool Audio::Initialize(IXAudio2* xAudio2, const SoundData& soundData, bool isLoop) {
        assert(xAudio2);

        HRESULT hr = xAudio2->CreateSourceVoice(&pSourceVoice_, &soundData.wfex);
        assert(SUCCEEDED(hr));

        XAUDIO2_BUFFER buf{};
        buf.pAudioData = soundData.pBuffer;
        buf.AudioBytes = soundData.bufferSize;
        buf.Flags = XAUDIO2_END_OF_STREAM;

        if (isLoop) {
            buf.LoopCount = XAUDIO2_LOOP_INFINITE;
        }

        hr = pSourceVoice_->SubmitSourceBuffer(&buf);
        assert(SUCCEEDED(hr));

        return true;
    }

    void Audio::Play() {
        if (!pSourceVoice_) return;
        HRESULT hr = pSourceVoice_->Start();
        assert(SUCCEEDED(hr));
    }

    void Audio::Stop() {
        if (!pSourceVoice_) return;
        HRESULT hr = pSourceVoice_->Stop();
        assert(SUCCEEDED(hr));
        hr = pSourceVoice_->FlushSourceBuffers();
        assert(SUCCEEDED(hr));
    }

    void Audio::Pause() {
        if (!pSourceVoice_) return;
        HRESULT hr = pSourceVoice_->Stop();
        assert(SUCCEEDED(hr));
    }

    void Audio::SetVolume(float volume) {
        if (!pSourceVoice_) return;
        HRESULT hr = pSourceVoice_->SetVolume(volume);
        assert(SUCCEEDED(hr));
    }

    bool Audio::IsPlaying() const {
        if (!pSourceVoice_) return false;
        XAUDIO2_VOICE_STATE state;
        pSourceVoice_->GetState(&state);
        return state.BuffersQueued > 0;
    }

}