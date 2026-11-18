#pragma once

#include <xaudio2.h>
#include "Sound.h"

namespace MTEngine {

    class Audio {
    public:
        Audio() = default;
        ~Audio();

        // 初期化（SourceVoiceの生成とデータ送信）
        bool Initialize(IXAudio2* xAudio2, const SoundData& soundData, bool isLoop = false);

        void Play();
        void Stop();
        void Pause();
        void SetVolume(float volume);

        // 再生が終了したかどうか
        bool IsPlaying() const;

    private:
        IXAudio2SourceVoice* pSourceVoice_ = nullptr;
    };

}