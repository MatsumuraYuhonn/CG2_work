#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include<cassert>
#include "Logger.h"
#include "Window.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")


int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {


	InitializeLogger();

	createLogFile();

	IDXGIFactory7* dxgiFactory = nullptr;

	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	assert(SUCCEEDED(hr));

	IDXGIAdapter4* useAdapter = nullptr;

	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
		IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; i++) {


		DXGI_ADAPTER_DESC3 adapterDesc{};

		hr = useAdapter->GetDesc3(&adapterDesc);

		assert(SUCCEEDED(hr));

		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {

			Log(ConvertString(std::format(L"Use Adapter!{}\n", adapterDesc.Description)));

			break;
		}


		useAdapter = nullptr;

	}

	assert(useAdapter != nullptr);


	ID3D12Device* device = nullptr;

	D3D_FEATURE_LEVEL featureLevels[] = {

		D3D_FEATURE_LEVEL_12_2,  D3D_FEATURE_LEVEL_12_1,  D3D_FEATURE_LEVEL_12_0,

	};

	const char* featureLevelStrings[] = {
		"12.2", "12.1", "12.0",
	};

	for (size_t i = 0; i < _countof(featureLevels); ++i) {

		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));

		if (SUCCEEDED(hr)) {

			Log(std::format("Feature Level : {}\n", featureLevelStrings[i]));

			break;
		}
	}

	assert(device != nullptr);
	Log("complete create D3D12Device!!!\n");

	HWND hwnd = CreateGameWindow();

	// メインループ
	MSG msg{};

	while (msg.message != WM_QUIT) {

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {

			// ゲームの処理



		}
	}

	return 0;

}