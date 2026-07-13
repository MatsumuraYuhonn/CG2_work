#pragma once
#include<xaudio2.h>
#pragma comment(lib, "xaudio2.lib")
#include<fstream>

namespace MTEngine {

	// RIFF形式のチャンクヘッダー
	struct ChunkHeader {
		char id[4];    // チャンクID ("RIFF", "fmt ", "data"など)
		int32_t size;  // チャンクサイズ
	};

	// RIFFファイルの先頭ヘッダー
	struct RiffHeader {
		ChunkHeader chunk;
		char type[4];  // ファイルタイプ ("WAVE")
	};

	// fmtチャンク（音声フォーマット情報）
	struct FormatChunk {
		ChunkHeader chunk;
		WAVEFORMATEX fmt;
	};

	// ロードされた音声データの保持構造体
	struct SoundData {
		WAVEFORMATEX wfex;     // 音声フォーマット情報
		BYTE* pBuffer;         // 波形データ本体へのポインタ
		unsigned int bufferSize; // データサイズ
	};

	// WAVファイルをメモリに読み込む
	SoundData SoundLoadWave(const char* filename);

	// メモリにロードされた音声データを解放する
	void SoundUnload(SoundData* soundData);

	// XAudio2を使用して音声データを再生する
	void SoundPlayWave(IXAudio2* xAudio2, const SoundData* soundData);

}