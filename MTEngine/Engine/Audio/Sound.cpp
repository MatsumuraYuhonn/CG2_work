#include "Sound.h"
#include <cassert>


namespace MTEngine {

	SoundData SoundLoadWave(const char* filename) {

		// ファイルオープン
		std::ifstream file;

		file.open(filename, std::ios_base::binary);

		assert(file.is_open());


		// .wavデータ読み込み
		RiffHeader riff;

		file.read((char*)&riff, sizeof(riff));

		if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
			assert(false);
		}

		if (strncmp(riff.type, "WAVE", 4) != 0) {
			assert(false);
		}


		FormatChunk format = {};

		file.read((char*)&format, sizeof(ChunkHeader));
		if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
			assert(false);
		}

		assert(format.chunk.size <= sizeof(format.fmt));
		file.read((char*)&format.fmt, format.chunk.size);


		ChunkHeader data;
		file.read((char*)&data, sizeof(data));

		if (strncmp(data.id, "JUNK", 4) == 0) {

			file.seekg(data.size, std::ios_base::cur);

			file.read((char*)&data, sizeof(data));
		}

		if (strncmp(data.id, "data", 4) != 0) {
			assert(false);
		}

		char* pBuffer = new char[data.size];
		file.read(pBuffer, data.size);


		file.close();


		// 読み込んだ音声データをreturn
		SoundData soundData = {};

		soundData.wfex = format.fmt;
		soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
		soundData.bufferSize = data.size;

		return soundData;

	}

	void SoundUnload(SoundData* soundData) {

		delete[] soundData->pBuffer;

		soundData->pBuffer = nullptr;
		soundData->bufferSize = 0;
		soundData->wfex = {};

	}

	void SoundPlayWave(IXAudio2* xAudio2, const SoundData* soundData) {

		HRESULT result;

		IXAudio2SourceVoice* pSourceVoice = nullptr;
		result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData->wfex);
		assert(SUCCEEDED(result));

		XAUDIO2_BUFFER buf{};
		buf.pAudioData = soundData->pBuffer;
		buf.AudioBytes = soundData->bufferSize;
		buf.Flags = XAUDIO2_END_OF_STREAM;

		result = pSourceVoice->SubmitSourceBuffer(&buf);
		result = pSourceVoice->Start();

	}

}