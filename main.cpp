#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include<cassert>
#include<dxgidebug.h>
#include<dxcapi.h>
#include<vector>
#include "Logger.h"
#include "Window.h"
#include "CrashHandler.h"

#include"externals/DirectXTex/DirectXTex.h"
#include"externals/DirectXTex/d3dx12.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

struct Vector4 {
	float x;
	float y;
	float z;
	float w;
};

struct Vector3 {
	float x;
	float y;
	float z;
};

struct Vector2 {
	float x;
	float y;
};

struct Matrix3x3 {
	float m[3][3];
};

struct Matrix4x4 {
	float m[4][4];
};

struct Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct DirectionalLight {
	Vector4 color;
	Vector3 direction;
	float intensity;
};

struct Material {
	Vector4 color;
	int32_t enabledLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};


Matrix4x4 MakeIdentity4x4() {
	Matrix4x4 result = {};

	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;

	return result;
}

Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
	Matrix4x4 result = MakeIdentity4x4();
	result.m[0][0] = scale.x;
	result.m[1][1] = scale.y;
	result.m[2][2] = scale.z;
	return result;
}

Matrix4x4 MakeRotateZMatrix(float theta) {
	Matrix4x4 result = MakeIdentity4x4(); 
	result.m[0][0] = std::cos(theta);
	result.m[0][1] = std::sin(theta);
	result.m[1][0] = -std::sin(theta);
	result.m[1][1] = std::cos(theta);
	return result;
}

Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result = MakeIdentity4x4(); 
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rot, const Vector3& translate) {
	Matrix4x4 result = {};

	// 回転の計算 (ラジアン角)
	float sinX = std::sin(rot.x);
	float cosX = std::cos(rot.x);
	float sinY = std::sin(rot.y);
	float cosY = std::cos(rot.y);
	float sinZ = std::sin(rot.z);
	float cosZ = std::cos(rot.z);

	// X軸 → Y軸 → Z軸の順で回転させた場合のアフィン変換行列
	// (Scale * Rotate * Translate)
	result.m[0][0] = scale.x * (cosY * cosZ + sinX * sinY * sinZ);
	result.m[0][1] = scale.x * (sinX * sinY * cosZ - cosY * sinZ);
	result.m[0][2] = scale.x * (cosX * sinY);
	result.m[0][3] = 0.0f;

	result.m[1][0] = scale.y * (cosX * sinZ);
	result.m[1][1] = scale.y * (cosX * cosZ);
	result.m[1][2] = scale.y * (-sinX);
	result.m[1][3] = 0.0f;

	result.m[2][0] = scale.z * (sinX * cosY * sinZ - sinY * cosZ);
	result.m[2][1] = scale.z * (sinX * cosY * cosZ + sinY * sinZ);
	result.m[2][2] = scale.z * (cosX * cosY);
	result.m[2][3] = 0.0f;

	// 平行移動
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;
	result.m[3][3] = 1.0f;

	return result;
}

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result = {};
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = m1.m[i][0] * m2.m[0][j] +
				m1.m[i][1] * m2.m[1][j] +
				m1.m[i][2] * m2.m[2][j] +
				m1.m[i][3] * m2.m[3][j];
		}
	}
	return result;
}

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
	Matrix4x4 result = {};

	float cot = 1.0f / std::tan(fovY / 2.0f);

	result.m[0][0] = cot / aspectRatio;
	result.m[1][1] = cot;
	result.m[2][2] = farClip / (farClip - nearClip);
	result.m[2][3] = 1.0f;
	result.m[3][2] = -(nearClip * farClip) / (farClip - nearClip);
	result.m[3][3] = 0.0f;

	return result;
}

Matrix4x4 Inverse(const Matrix4x4& m) {
	Matrix4x4 result = {};

	float b00 = m.m[0][0] * m.m[1][1] - m.m[0][1] * m.m[1][0];
	float b01 = m.m[0][0] * m.m[1][2] - m.m[0][2] * m.m[1][0];
	float b02 = m.m[0][0] * m.m[1][3] - m.m[0][3] * m.m[1][0];
	float b03 = m.m[0][1] * m.m[1][2] - m.m[0][2] * m.m[1][1];
	float b04 = m.m[0][1] * m.m[1][3] - m.m[0][3] * m.m[1][1];
	float b05 = m.m[0][2] * m.m[1][3] - m.m[0][3] * m.m[1][2];
	float b06 = m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0];
	float b07 = m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0];
	float b08 = m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0];
	float b09 = m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1];
	float b10 = m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1];
	float b11 = m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2];

	float det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;

	if (det == 0.0f) {
		return result;
	}

	float invDet = 1.0f / det;

	result.m[0][0] = (m.m[1][1] * b11 - m.m[1][2] * b10 + m.m[1][3] * b09) * invDet;
	result.m[0][1] = (-m.m[0][1] * b11 + m.m[0][2] * b10 - m.m[0][3] * b09) * invDet;
	result.m[0][2] = (m.m[3][1] * b05 - m.m[3][2] * b04 + m.m[3][3] * b03) * invDet;
	result.m[0][3] = (-m.m[2][1] * b05 + m.m[2][2] * b04 - m.m[2][3] * b03) * invDet;

	result.m[1][0] = (-m.m[1][0] * b11 + m.m[1][2] * b08 - m.m[1][3] * b07) * invDet;
	result.m[1][1] = (m.m[0][0] * b11 - m.m[0][2] * b08 + m.m[0][3] * b07) * invDet;
	result.m[1][2] = (-m.m[3][0] * b05 + m.m[3][2] * b02 - m.m[3][3] * b01) * invDet;
	result.m[1][3] = (m.m[2][0] * b05 - m.m[2][2] * b02 + m.m[2][3] * b01) * invDet;

	result.m[2][0] = (m.m[1][0] * b10 - m.m[1][1] * b08 + m.m[1][3] * b06) * invDet;
	result.m[2][1] = (-m.m[0][0] * b10 + m.m[0][1] * b08 - m.m[0][3] * b06) * invDet;
	result.m[2][2] = (m.m[3][0] * b04 - m.m[3][1] * b02 + m.m[3][3] * b00) * invDet;
	result.m[2][3] = (-m.m[2][0] * b04 + m.m[2][1] * b02 - m.m[2][3] * b00) * invDet;

	result.m[3][0] = (-m.m[1][0] * b09 + m.m[1][1] * b07 - m.m[1][2] * b06) * invDet;
	result.m[3][1] = (m.m[0][0] * b09 - m.m[0][1] * b07 + m.m[0][2] * b06) * invDet;
	result.m[3][2] = (-m.m[3][0] * b03 + m.m[3][1] * b01 - m.m[3][2] * b00) * invDet;
	result.m[3][3] = (m.m[2][0] * b03 - m.m[2][1] * b01 + m.m[2][2] * b00) * invDet;

	return result;
}

IDxcBlob* CompileShader(const std::wstring& filePath, const wchar_t* profile, IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler, IDxcIncludeHandler* includeHandler) {


	// ファイルを読む
	Log(ConvertString(std::format(L"Begin CompileShader, Path:{}, profile:{}\n", filePath, profile)));

	IDxcBlobEncoding* shaderSource = nullptr;

	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);

	assert(SUCCEEDED(hr));

	DxcBuffer shaderSourceBuffer{};
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;


	// コンパイルする
	LPCWSTR arguments[] = {

		filePath.c_str(),
		L"-E", L"main",
		L"-T", profile,
		L"-Zi", L"-Qembed_debug",
		L"-Od",
		L"-Zpr"
	};

	IDxcResult* shaderResult = nullptr;

	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler,
		IID_PPV_ARGS(&shaderResult)
	);

	assert(SUCCEEDED(hr));


	// 警告・エラーが出てないか確認する
	IDxcBlobUtf8* shaderError = nullptr;

	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);

	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {

		Log(shaderError->GetStringPointer());

		assert(false);

	}


	// コンパイル結果を受け取って返す
	IDxcBlob* shaderBlob = nullptr;

	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);

	assert(SUCCEEDED(hr));

	Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));

	shaderSource->Release();
	shaderResult->Release();

	return shaderBlob;

}


ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {

	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferResourceDesc{};
	bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferResourceDesc.Width = sizeInBytes; 
	bufferResourceDesc.Height = 1;
	bufferResourceDesc.DepthOrArraySize = 1;
	bufferResourceDesc.MipLevels = 1;
	bufferResourceDesc.SampleDesc.Count = 1;
	bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* bufferResource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&bufferResource)
	);
	assert(SUCCEEDED(hr));

	return bufferResource;
}

ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
	
	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

// Textureデータを読む
DirectX::ScratchImage LoadTexture(const std::string& filePath) {
	
	DirectX::ScratchImage image{};

	std::wstring filePathw = ConvertString(filePath);

	HRESULT hr = DirectX::LoadFromWICFile(filePathw.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);

	assert(SUCCEEDED(hr));


	DirectX::ScratchImage mipImage{};

	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImage);

	assert(SUCCEEDED(hr));

	return mipImage;
}

// TextureResourceを作る
ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata) {
	

	// metadataをもとにresourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};

	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);


	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	//heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	//heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;



	// Resourceの生成
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&resource)
	);

	assert(SUCCEEDED(hr));

	return resource;

}

// TextureResourceにデータを転送する
[[nodiscard]]
ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList)
{
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	ID3D12Resource* intermediateResource = CreateBufferResource(device, intermediateSize);
	UpdateSubresources(commandList, texture, intermediateResource, 0, 0, UINT(subresources.size()), subresources.data());
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}

ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height) {

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width; 
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; 
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; 
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; 


	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; 


	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f; 
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; 

	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, 
		&depthClearValue,
		IID_PPV_ARGS(&resource)
	);

	assert(SUCCEEDED(hr));

	return resource;
}

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
	Matrix4x4 result = { 0 };

	result.m[0][0] = 2.0f / (right - left);
	result.m[1][1] = 2.0f / (top - bottom);
	result.m[2][2] = 1.0f / (farClip - nearClip);

	result.m[3][0] = -(right + left) / (right - left);
	result.m[3][1] = -(top + bottom) / (top - bottom);
	result.m[3][2] = -nearClip / (farClip - nearClip);
	result.m[3][3] = 1.0f;

	return result;
}

D3D12_CPU_DESCRIPTOR_HANDLE getCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {

	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);

	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE getGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {

	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);

	return handleGPU;
}

inline size_t AlignForConstantBuffer(size_t size) {
	return (size + 255) & ~255;
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	HRESULT hrCoInit = CoInitializeEx(0, COINIT_MULTITHREADED);
	assert(SUCCEEDED(hrCoInit));

	SetUnhandledExceptionFilter(ExportDump);

	InitializeLogger();

	createLogFile();

	Log("String\n");

	HWND hwnd = CreateGameWindow();


#ifdef _DEBUG

	ID3D12Debug1* debugController = nullptr;

	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {

		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(TRUE);

	}

#endif


	// デバイスの作成
	IDXGIFactory7* dxgiFactory = nullptr;

	HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory));

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


	const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


#ifdef _DEBUG

	ID3D12InfoQueue* infoQueue = nullptr;

	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {

		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		
		D3D12_MESSAGE_ID denyIds[] = {

			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
		
		};

		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };

		D3D12_INFO_QUEUE_FILTER filter{};

		filter.DenyList.NumIDs = _countof(denyIds);

		filter.DenyList.pIDList = denyIds;

		filter.DenyList.NumSeverities = _countof(severities);

		filter.DenyList.pSeverityList = severities;

		infoQueue->PushStorageFilter(&filter);

		infoQueue->Release();

	}

#endif


	// CommandQueueの作成
	ID3D12CommandQueue* commandQueue = nullptr;

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};

	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));

	assert(SUCCEEDED(hr));


	// コマンドアロケータの作成
	ID3D12CommandAllocator* commandAllocator = nullptr;

	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));

	assert(SUCCEEDED(hr));


	// コマンドリストの作成
	ID3D12GraphicsCommandList* commandList = nullptr;

	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));

	assert(SUCCEEDED(hr));


	// スワップチェーンの作成
	IDXGISwapChain4* swapChain = nullptr;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth;
	swapChainDesc.Height = kClientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr,
		reinterpret_cast<IDXGISwapChain1**>(&swapChain));

	assert(SUCCEEDED(hr));


	// DescriptorHeapの作成
	ID3D12DescriptorHeap* rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

	ID3D12DescriptorHeap* srvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	// スワップチェーンからリソースを引っ張ってくる
	ID3D12Resource* swapChainResources[2] = { nullptr };

	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));

	assert(SUCCEEDED(hr));

	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));

	assert(SUCCEEDED(hr));


	// RTVの作成
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2] = {};

	rtvHandles[0] = getCPUDescriptorHandle(rtvDescriptorHeap, descriptorSizeRTV, 0);
	device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);

	rtvHandles[1] = getCPUDescriptorHandle(rtvDescriptorHeap, descriptorSizeRTV, 1);
	device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);


	ID3D12Resource* depthStencilResource = CreateDepthStencilTextureResource(device, kClientWidth, kClientHeight);


	ID3D12DescriptorHeap* dsvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; 
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; 
	device->CreateDepthStencilView(depthStencilResource, &dsvDesc, getCPUDescriptorHandle(dsvDescriptorHeap, descriptorSizeDSV, 0));


	// フェンスの作成
	ID3D12Fence* fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);


	// rootSignatureの作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};

	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;



	D3D12_ROOT_PARAMETER rootParameters[4] = {};

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;

	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParameters[0].Descriptor.ShaderRegister = 0;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;

	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	rootParameters[1].Descriptor.ShaderRegister = 1;


	// DescriptorRange
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0; 
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);


	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;   
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1;

	descriptionRootSignature.pParameters = rootParameters;

	descriptionRootSignature.NumParameters = _countof(rootParameters);


	// Samplerqの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	ID3DBlob* signatureBlob = nullptr;

	ID3D10Blob* errorBlob = nullptr;

	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

	if (FAILED(hr)) {

		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));

		assert(false);

	}

	ID3D12RootSignature* rootSignature = nullptr;
	
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	
	assert(SUCCEEDED(hr));


	// インプットレイアウトの設定
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};

	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;




	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};

	inputLayoutDesc.pInputElementDescs = inputElementDescs;

	inputLayoutDesc.NumElements = _countof(inputElementDescs);


	// BlandStateの設定
	D3D12_BLEND_DESC blendDesc{};

	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;


	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;


	// DepthStencilStateの設定
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;

	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	IDxcIncludeHandler* includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));


	// シェーダーのコンパイル
	IDxcBlob* vertexShaderBlob = CompileShader(L"object3d.VS.hlsl", L"vs_6_0",
		dxcUtils, dxcCompiler, includeHandler);

	assert(vertexShaderBlob != nullptr);

	IDxcBlob* pixelShaderBlob = CompileShader(L"object3d.PS.hlsl", L"ps_6_0",
		dxcUtils, dxcCompiler, includeHandler);

	assert(pixelShaderBlob != nullptr);


	// PSOを生成する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};

	graphicsPipelineStateDesc.pRootSignature= rootSignature;

	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;

	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };

	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };

	graphicsPipelineStateDesc.BlendState = blendDesc;

	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

	graphicsPipelineStateDesc.NumRenderTargets = 1;

	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	graphicsPipelineStateDesc.SampleDesc.Count = 1;

	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;


	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	
	depthStencilDesc.DepthEnable = true;
	
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;


	ID3D12PipelineState* graphicsPipelineState = nullptr;

	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));

	assert(SUCCEEDED(hr));


	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};

	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC vertexResourceDesc{};

	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

	vertexResourceDesc.Width = sizeof(VertexData) * 3;

	vertexResourceDesc.Height = 1;

	vertexResourceDesc.DepthOrArraySize = 1;

	vertexResourceDesc.MipLevels = 1;

	vertexResourceDesc.SampleDesc.Count = 1;

	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;


	const uint32_t kSubdivision = 16;
	const uint32_t kVertexCountSphere = (kSubdivision + 1) * (kSubdivision + 1);
	const uint32_t kIndexCountSphere = kSubdivision * kSubdivision * 6;

	//　球体用の頂点リソース
	ID3D12Resource* vertexResource = CreateBufferResource(device, sizeof(VertexData) * kVertexCountSphere);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(VertexData) * kVertexCountSphere;
	vertexBufferView.StrideInBytes = sizeof(VertexData);


	// 球体用のインデックスリソース
	ID3D12Resource* indexResourceSphere = CreateBufferResource(device, sizeof(uint32_t) * kIndexCountSphere);

	D3D12_INDEX_BUFFER_VIEW indexBufferViewSphere{};
	indexBufferViewSphere.BufferLocation = indexResourceSphere->GetGPUVirtualAddress();
	indexBufferViewSphere.SizeInBytes = sizeof(uint32_t) * kIndexCountSphere;
	indexBufferViewSphere.Format = DXGI_FORMAT_R32_UINT;


	// 頂点データの書き込み
	VertexData* vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	const float pi = 3.1415926535f;
	for (uint32_t latIndex = 0; latIndex <= kSubdivision; ++latIndex) {

		float lat = -pi / 2.0f + (pi / float(kSubdivision)) * latIndex;

		for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; ++lonIndex) {

			uint32_t index = latIndex * (kSubdivision + 1) + lonIndex;
			float lon = lonIndex * (pi * 2.0f / float(kSubdivision));

			vertexData[index].position = { cos(lat) * cos(lon), sin(lat), cos(lat) * sin(lon), 1.0f };
			vertexData[index].texcoord = { float(lonIndex) / float(kSubdivision), 1.0f - float(latIndex) / float(kSubdivision) };
			vertexData[index].normal = { vertexData[index].position.x, vertexData[index].position.y, vertexData[index].position.z };
		}
	}

	uint32_t* indexDataSphere = nullptr;
	indexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSphere));

	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {

		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {

			uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;
			uint32_t v0 = latIndex * (kSubdivision + 1) + lonIndex;
			uint32_t v1 = v0 + 1;
			uint32_t v2 = (latIndex + 1) * (kSubdivision + 1) + lonIndex;
			uint32_t v3 = v2 + 1;

			indexDataSphere[start] = v0; indexDataSphere[start + 1] = v1; indexDataSphere[start + 2] = v2;
			indexDataSphere[start + 3] = v1; indexDataSphere[start + 4] = v3; indexDataSphere[start + 5] = v2;
		}
	}

	indexResourceSphere->Unmap(0, nullptr);





	// スプライト用の頂点リソースを作る
	ID3D12Resource* vertexResourceSprite = CreateBufferResource(device, sizeof(VertexData) * 4);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 4;
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);


	// スプライト用のインデックスリソースを作る
	ID3D12Resource* indexResourceSprite = CreateBufferResource(device, sizeof(uint32_t) * 6);

	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};

	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;


	// インデックスリソースにデータを書き込む
	uint32_t* indexDataSprite = nullptr;

	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0;
	indexDataSprite[1] = 1;
	indexDataSprite[2] = 2;
	indexDataSprite[3] = 1;
	indexDataSprite[4] = 3;
	indexDataSprite[5] = 2;


	// 頂点データを設定する
	VertexData* vertexDataSprite = nullptr;
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));

	vertexDataSprite[0].position = { 0.0f, 360.0f, 0.0f, 1.0f };
	vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
	vertexDataSprite[0].normal = { 0.0f, 0.0f, -1.0f };

	vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	vertexDataSprite[1].texcoord = { 0.0f, 0.0f };
	vertexDataSprite[1].normal = { 0.0f, 0.0f, -1.0f };

	vertexDataSprite[2].position = { 640.0f, 360.0f, 0.0f, 1.0f }; 
	vertexDataSprite[2].texcoord = { 1.0f, 1.0f };
	vertexDataSprite[2].normal = { 0.0f, 0.0f, -1.0f };

	vertexDataSprite[3].position = { 640.0f, 0.0f, 0.0f, 1.0f };
	vertexDataSprite[3].texcoord = { 1.0f, 0.0f };
	vertexDataSprite[3].normal = { 0.0f, 0.0f, -1.0f };

	//vertexDataSprite[4].position = { 640.0f, 0.0f, 0.0f, 1.0f };
	//vertexDataSprite[4].texcoord = { 1.0f, 0.0f };
	//vertexDataSprite[4].normal = { 0.0f, 0.0f, -1.0f };

	//vertexDataSprite[5].position = { 640.0f, 360.0f, 0.0f, 1.0f };
	//vertexDataSprite[5].texcoord = { 1.0f, 1.0f };
	//vertexDataSprite[5].normal = { 0.0f, 0.0f, -1.0f };


	// マテリアル用のリソースを作る
	ID3D12Resource* materialResource = CreateBufferResource(device, AlignForConstantBuffer(sizeof(Material)));

	Material* materialData = nullptr;

	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

	materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };

	materialData->enabledLighting = 1; 

	materialData->uvTransform = MakeIdentity4x4();


	ID3D12Resource* materialResourceSprite = CreateBufferResource(device, AlignForConstantBuffer(sizeof(Material)));
	Material* materialDataSprite = nullptr;
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));

	materialDataSprite->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialDataSprite->enabledLighting = false;
	materialDataSprite->uvTransform = MakeIdentity4x4();

	ID3D12Resource* directionalLightResource = CreateBufferResource(device, AlignForConstantBuffer(sizeof(DirectionalLight)));

	DirectionalLight* directionalLightData = nullptr;
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { -1.0f, 0.0f, 0.0f };
	directionalLightData->intensity = 1.0f;


	// トランスフォーメーションマトリックス用のリソースを作る
	ID3D12Resource* wvpResource = CreateBufferResource(device, AlignForConstantBuffer(sizeof(Matrix4x4)));

	TransformationMatrix* wvpData = nullptr;

	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));

	wvpData->WVP = MakeIdentity4x4();
	wvpData->World = MakeIdentity4x4();


	// Sprite用のTransformationMatrix用のリソースを作る。
	ID3D12Resource* transformationMatrixResourceSprite = CreateBufferResource(device, AlignForConstantBuffer(sizeof(Matrix4x4)));
	
	Matrix4x4* transformationMatrixDataSprite = nullptr;
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
	*transformationMatrixDataSprite = MakeIdentity4x4();


	// textureを読んで転送する
	DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	
	ID3D12Resource* textureResource = CreateTextureResource(device, metadata);
	
	ID3D12Resource* intermediateResource = UploadTextureData(textureResource, mipImages, device, commandList);


	// 2枚目のtextureを読む
	DirectX::ScratchImage mipImages2 = LoadTexture("resources/monsterBall.png");

	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();

	ID3D12Resource* textureResource2 = CreateTextureResource(device, metadata2);

	ID3D12Resource* intermediateResource2 = UploadTextureData(textureResource2, mipImages2, device, commandList);

	bool useMonsterBall = true;

	



	// shaderResourceViewを作る
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = getCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = getGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);

	//device->CreateShaderResourceView(textureResource, &srvDesc, textureSrvHandleCPU);

	UINT srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	textureSrvHandleCPU.ptr += srvDescriptorSize; 

	device->CreateShaderResourceView(textureResource, &srvDesc, textureSrvHandleCPU);



	// 2枚目のSRVを作る
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};

	srvDesc2.Format = metadata2.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = getCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = getGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);

	device->CreateShaderResourceView(textureResource2, &srvDesc2, textureSrvHandleCPU2);


	// ViewPortとScissorの設定

	D3D12_VIEWPORT viewport{};

	viewport.Width = kClientWidth;

	viewport.Height = kClientHeight;

	viewport.TopLeftX = 0;

	viewport.TopLeftY = 0;

	viewport.MinDepth = 0.0f;

	viewport.MaxDepth = 1.0f;


	D3D12_RECT scissorRect{};

	scissorRect.left = 0;

	scissorRect.right = kClientWidth;

	scissorRect.top = 0;

	scissorRect.bottom = kClientHeight;

	

	// transform変数を作る
	Transform transform{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f},  {0.0f,0.0f,0.0f} };

	Transform cameraTransform{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f},  {0.0f,0.0f,-10.0f} };

	Transform transformSprite{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

	Transform uvTransformSprite{
	{ 1.0f, 1.0f, 1.0f },
	{ 0.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f }, 
	};

#ifdef USE_IMGUI

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(device, swapChainDesc.BufferCount, rtvDesc.Format, srvDescriptorHeap,
		getCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 0), getGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 0));
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Build();

#endif

	// メインループ
	MSG msg{};

	while (msg.message != WM_QUIT) {

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {

#ifdef USE_IMGUI

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

#endif

			// ゲームの処理
			transform.rotate.y += 0.03f;

			Vector3 cameraScale = { 1.0f, 1.0f, 1.0f };
			Vector3 cameraRot = { 0.0f, 0.0f, 0.0f };
			Vector3 cameraPos = { 0.0f, 0.0f, -5.0f }; 

			Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
			Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
	
			//*wvpData = worldViewProjectionMatrix;
			wvpData->WVP = worldViewProjectionMatrix;
			wvpData->World = worldMatrix;

			// Sprite用のWorldViewProjectionMatrixを作る
			Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
			Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
			Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(kClientWidth), float(kClientHeight), 0.0f, 100.0f);
			Matrix4x4 worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
			*transformationMatrixDataSprite = worldViewProjectionMatrixSprite;

			// UV変換用のマトリックスを作る
			Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
			uvTransformMatrix = Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransformSprite.rotate.z));
			uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
			materialDataSprite->uvTransform = uvTransformMatrix;


#ifdef USE_IMGUI

			ImGui::Begin("Debug Settings");

			ImGui::DragFloat3("Camera Position", &cameraTransform.translate.x, 0.1f);
			ImGui::DragFloat3("Camera Rotation", &cameraTransform.rotate.x, 0.01f);

			ImGui::DragFloat3("Sprite Position", &transformSprite.translate.x, 1.0f);
			ImGui::DragFloat3("Sprite Rotation", &transformSprite.rotate.x, 0.01f);
			ImGui::DragFloat3("Sprite Scale", &transformSprite.scale.x, 0.01f);

			ImGui::Checkbox("useMonsterBall", &useMonsterBall);


			ImGui::ColorEdit4("Light Color", &directionalLightData->color.x);

			if (ImGui::DragFloat3("Light Direction", &directionalLightData->direction.x, 0.01f, -1.0f, 1.0f)) {
	
				float length = std::sqrt(directionalLightData->direction.x * directionalLightData->direction.x +
					directionalLightData->direction.y * directionalLightData->direction.y +
					directionalLightData->direction.z * directionalLightData->direction.z);

				if (length != 0) {
					directionalLightData->direction.x /= length;
					directionalLightData->direction.y /= length;
					directionalLightData->direction.z /= length;
				}
			}

			ImGui::DragFloat("Intensity", &directionalLightData->intensity, 0.01f, 0.0f, 10.0f);


			ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
			ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
			ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);


			ImGui::End();
			
			ImGui::Render();

#endif

			// コマンドを積み込んで確定させる
			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

			// バリアの設定
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = swapChainResources[backBufferIndex];
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

			commandList->ResourceBarrier(1, &barrier);

			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
			float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

			commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


			// 描画コマンドを積む
			commandList->RSSetViewports(1, &viewport);

			commandList->RSSetScissorRects(1, &scissorRect);

			commandList->SetGraphicsRootSignature(rootSignature);

			commandList->SetPipelineState(graphicsPipelineState);

			commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

			commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

			//ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
			//commandList->SetDescriptorHeaps(1, descriptorHeaps);
			//commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
			//commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
			//commandList->DrawInstanced(1536, 1, 0, 0);

			ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
			commandList->SetDescriptorHeaps(1, descriptorHeaps); 

			// --- 球体の描画 ---
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
			commandList->IASetIndexBuffer(&indexBufferViewSphere);
			commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

			commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
			commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
			commandList->DrawIndexedInstanced(kIndexCountSphere, 1, 0, 0, 0);

			// --- スプライトの描画 ---
			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
			commandList->IASetIndexBuffer(&indexBufferViewSprite);
			commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

			commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
			commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);


#ifdef USE_IMGUI

			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

#endif

			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

			commandList->ResourceBarrier(1, &barrier);

			hr = commandList->Close();
			assert(SUCCEEDED(hr));


			// コマンドをキックする
			ID3D12CommandList* commandLists[] = { commandList };

			commandQueue->ExecuteCommandLists(1, commandLists);

			swapChain->Present(1, 0);


			fenceValue++;

			commandQueue->Signal(fence, fenceValue);

			if (fence->GetCompletedValue() < fenceValue) {

				fence->SetEventOnCompletion(fenceValue, fenceEvent);

				WaitForSingleObject(fenceEvent, INFINITE);
			}


			hr = commandAllocator->Reset();
			assert(SUCCEEDED(hr));

			hr = commandList->Reset(commandAllocator, nullptr);
			assert(SUCCEEDED(hr));

		}
	}

#ifdef USE_IMGUI

	ImGui_ImplWin32_Shutdown();
	ImGui_ImplDX12_Shutdown();
	ImGui::DestroyContext();

#endif
	intermediateResource2->Release();
	textureResource2->Release();
	intermediateResource->Release();
	textureResource->Release();
	srvDescriptorHeap->Release();
	depthStencilResource->Release();
	dsvDescriptorHeap->Release();
	materialResourceSprite->Release();
	directionalLightResource->Release(); 
	materialResource->Release();
	wvpResource->Release();
	vertexResource->Release();
	vertexResourceSprite->Release();
	indexResourceSprite->Release();
	indexResourceSphere->Release();
	transformationMatrixResourceSprite->Release();
	graphicsPipelineState->Release();
	pixelShaderBlob->Release();
	vertexShaderBlob->Release();
	rootSignature->Release();
	if (errorBlob) {
		errorBlob->Release();
	}
	signatureBlob->Release();
	CloseHandle(fenceEvent);
	fence->Release();
	swapChainResources[1]->Release();
	swapChainResources[0]->Release();
	rtvDescriptorHeap->Release();
	swapChain->Release();
	commandList->Release();
	commandAllocator->Release();
	commandQueue->Release();
	device->Release();
	dxcCompiler->Release();
	dxcUtils->Release();
	includeHandler->Release();
	useAdapter->Release();
	dxgiFactory->Release();
	CoUninitialize();

#ifdef _DEBUG
	debugController->Release();
#endif 

	CloseWindow(hwnd);

	IDXGIDebug1* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}

	return 0;

}