#pragma once
#include<xaudio2.h>
#pragma comment(lib, "xaudio2.lib")
#include<fstream>

struct ChunkHeader {
	char id[4];
	int32_t size;
};

struct RiffHeader {
	ChunkHeader chunk;
	char type[4];
};

struct FormatChunk {
	ChunkHeader chunk;
	WAVEFORMATEX fmt;
};

struct SoundData {
	WAVEFORMATEX wfex;
	BYTE* pBuffer;
	unsigned int bufferSize;
};


SoundData SoundLoadWave(const char* filename);

void SoundUnload(SoundData* soundData);

void SoundPlayWave(IXAudio2* xAudio2, const SoundData* soundData);
