#include "Sound.h"
#include <cassert>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <vector>
#include <wrl.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")


namespace MTEngine {
	namespace {
		SoundData SoundLoadMediaFoundation(const char* filename) {
			const int32_t wideLength = MultiByteToWideChar(
				CP_UTF8, 0, filename, -1, nullptr, 0);
			assert(wideLength > 0);
			std::wstring widePath(static_cast<size_t>(wideLength), L'\0');
			MultiByteToWideChar(
				CP_UTF8, 0, filename, -1, widePath.data(), wideLength);

			Microsoft::WRL::ComPtr<IMFSourceReader> sourceReader;
			HRESULT hr = MFCreateSourceReaderFromURL(
				widePath.c_str(), nullptr, sourceReader.GetAddressOf());
			assert(SUCCEEDED(hr));
			if (FAILED(hr)) {
				return {};
			}

			Microsoft::WRL::ComPtr<IMFMediaType> requestedType;
			hr = MFCreateMediaType(requestedType.GetAddressOf());
			assert(SUCCEEDED(hr));
			hr = requestedType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
			assert(SUCCEEDED(hr));
			hr = requestedType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
			assert(SUCCEEDED(hr));
			hr = sourceReader->SetCurrentMediaType(
				MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, requestedType.Get());
			assert(SUCCEEDED(hr));

			Microsoft::WRL::ComPtr<IMFMediaType> decodedType;
			hr = sourceReader->GetCurrentMediaType(
				MF_SOURCE_READER_FIRST_AUDIO_STREAM, decodedType.GetAddressOf());
			assert(SUCCEEDED(hr));

			WAVEFORMATEX* decodedFormat = nullptr;
			UINT32 decodedFormatSize = 0;
			hr = MFCreateWaveFormatExFromMFMediaType(
				decodedType.Get(), &decodedFormat, &decodedFormatSize);
			assert(SUCCEEDED(hr));
			assert(decodedFormat && decodedFormat->wFormatTag == WAVE_FORMAT_PCM);

			std::vector<BYTE> decodedBytes;
			while (true) {
				DWORD streamFlags = 0;
				Microsoft::WRL::ComPtr<IMFSample> sample;
				hr = sourceReader->ReadSample(
					MF_SOURCE_READER_FIRST_AUDIO_STREAM,
					0,
					nullptr,
					&streamFlags,
					nullptr,
					sample.GetAddressOf());
				assert(SUCCEEDED(hr));
				if (FAILED(hr) || (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM)) {
					break;
				}
				if (!sample) {
					continue;
				}

				Microsoft::WRL::ComPtr<IMFMediaBuffer> mediaBuffer;
				hr = sample->ConvertToContiguousBuffer(mediaBuffer.GetAddressOf());
				assert(SUCCEEDED(hr));
				BYTE* sourceBytes = nullptr;
				DWORD currentLength = 0;
				hr = mediaBuffer->Lock(&sourceBytes, nullptr, &currentLength);
				assert(SUCCEEDED(hr));
				decodedBytes.insert(
					decodedBytes.end(), sourceBytes, sourceBytes + currentLength);
				mediaBuffer->Unlock();
			}

			SoundData soundData{};
			soundData.wfex = *decodedFormat;
			soundData.wfex.cbSize = 0;
			CoTaskMemFree(decodedFormat);
			soundData.bufferSize = static_cast<unsigned int>(decodedBytes.size());
			soundData.pBuffer = new BYTE[soundData.bufferSize];
			std::copy(decodedBytes.begin(), decodedBytes.end(), soundData.pBuffer);
			return soundData;
		}
	}

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

	SoundData SoundLoad(const char* filename) {
		std::string extension = std::filesystem::path(filename).extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(),
			[](unsigned char character) {
				return static_cast<char>(std::tolower(character));
			});
		if (extension == ".wav") {
			return SoundLoadWave(filename);
		}
		return SoundLoadMediaFoundation(filename);
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
