#include <Windows.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <wrl.h>

// ファイル分け済み
#include "Logger.h"
#include "Window.h"
#include "Vector.h"
#include "DeviceInput.h"
#include "Sound.h"
#include "Dx12Device.h"
#include "Transform.h"
#include "CrashHandler.h"
#include "DebugCamera.h"
#include "Model.h"
#include "Sprite.h"
#include "ShaderCompiler.h"
#include "ConstantBuffer.h"
#include "SwapChain.h"
#include "DescriptorHeapManager.h"


#include"externals/DirectXTex/DirectXTex.h"
#include"externals/DirectXTex/d3dx12.h"

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")


struct D3DResourceLeakChecker {
	~D3DResourceLeakChecker() {
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};

class ResourceObject {

public:

	ResourceObject(Microsoft::WRL::ComPtr <ID3D12Resource> resource) :resource_(resource) {}

	~ResourceObject() {


	};

	Microsoft::WRL::ComPtr <ID3D12Resource> Get() { return resource_; }

private:

	Microsoft::WRL::ComPtr <ID3D12Resource> resource_;

};

Microsoft::WRL::ComPtr <ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr <ID3D12Device> device, size_t sizeInBytes) {

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

	Microsoft::WRL::ComPtr <ID3D12Resource> bufferResource = nullptr;
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
Microsoft::WRL::ComPtr <ID3D12Resource> CreateTextureResource(Microsoft::WRL::ComPtr <ID3D12Device> device, const DirectX::TexMetadata& metadata) {

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

	// Resourceの生成
	Microsoft::WRL::ComPtr <ID3D12Resource> resource = nullptr;
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
Microsoft::WRL::ComPtr <ID3D12Resource> UploadTextureData(Microsoft::WRL::ComPtr <ID3D12Resource> texture, const DirectX::ScratchImage& mipImages,
	Microsoft::WRL::ComPtr <ID3D12Device> device, Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> commandList)
{
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
	Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource = CreateBufferResource(device.Get(), intermediateSize);
	UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}

Microsoft::WRL::ComPtr <ID3D12Resource> CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr <ID3D12Device> device, int32_t width, int32_t height) {

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

	Microsoft::WRL::ComPtr <ID3D12Resource> resource = nullptr;
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


inline size_t AlignForConstantBuffer(size_t size) {
	return (size + 255) & ~255;
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	D3DResourceLeakChecker leakCheck;

	HRESULT hrCoInit = CoInitializeEx(0, COINIT_MULTITHREADED);
	assert(SUCCEEDED(hrCoInit));

	SetUnhandledExceptionFilter(ExportDump);

	InitializeLogger();

	createLogFile();

	Log("String\n");

	hwnd = CreateGameWindow();

	DeviceInputInitialize(GetModuleHandle(nullptr), hwnd);

#ifdef _DEBUG

	Microsoft::WRL::ComPtr <ID3D12Debug1> debugController = nullptr;

	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {

		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(TRUE);

	}

#endif

	// DirectX12の初期化
	Dx12Device dx12Device;
	bool isInitialized = dx12Device.Initialize();
	assert(isInitialized);

	Microsoft::WRL::ComPtr<ID3D12Device> device = dx12Device.GetDevice();
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = dx12Device.GetCommandList();
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = dx12Device.GetCommandAllocator();
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = dx12Device.GetCommandQueue();
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory = dx12Device.GetDxgiFactory();

	// DescriptorHeapManagerの初期化
	DescriptorHeapManager rtvHeapManager;
	DescriptorHeapManager dsvHeapManager;
	DescriptorHeapManager srvHeapManager;

	rtvHeapManager.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	dsvHeapManager.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	srvHeapManager.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);


#ifdef _DEBUG

	Microsoft::WRL::ComPtr <ID3D12InfoQueue> infoQueue = nullptr;

	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {

		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

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

	}

#endif

	// スワップチェーンの作成
	SwapChain swapChain;
	bool isSwapChainInit = swapChain.Initialize(dxgiFactory, commandQueue, hwnd, kClientWidth, kClientHeight);
	assert(isSwapChainInit);


	// RTVの作成
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2] = {};

	rtvHandles[0] = rtvHeapManager.GetCPUDescriptorHandle(0);
	device->CreateRenderTargetView(swapChain.GetBuffer(0).Get(), &rtvDesc, rtvHandles[0]);

	rtvHandles[1] = rtvHeapManager.GetCPUDescriptorHandle(1);
	device->CreateRenderTargetView(swapChain.GetBuffer(1).Get(), &rtvDesc, rtvHandles[1]);


	ResourceObject depthStencilResource = CreateDepthStencilTextureResource(device, kClientWidth, kClientHeight);
	
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	device->CreateDepthStencilView(depthStencilResource.Get().Get(), &dsvDesc, dsvHeapManager.GetCPUDescriptorHandle(0));



	// rootSignatureの作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};

	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;



	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[0].Descriptor.ShaderRegister = 0;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[1].Descriptor.ShaderRegister = 1;


	// DescriptorRange
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[2].Descriptor.ShaderRegister = 2;

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[3].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // t0


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

		if (errorBlob) {
			OutputDebugStringA(
				reinterpret_cast<char*>(errorBlob->GetBufferPointer())
			);
		}

		assert(false);

	}

	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature = nullptr;

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


	// ShaderCompilerの初期化
	std::unique_ptr<ShaderCompiler> shaderCompiler = std::make_unique<ShaderCompiler>();
	bool isShaderCompilerInit = shaderCompiler->Initialize();
	assert(isShaderCompilerInit);


	// シェーダーのコンパイル
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = shaderCompiler->Compile(L"object3d.VS.hlsl", L"vs_6_0");

	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = shaderCompiler->Compile(L"object3d.PS.hlsl", L"ps_6_0");

	assert(pixelShaderBlob != nullptr);


	// PSOを生成する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};

	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();

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


	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicsPipelineState = nullptr;

	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));

	assert(SUCCEEDED(hr));


	// 頂点リソース用のヒープの設定
	const uint32_t kSubdivision = 16;
	const uint32_t kVertexCountSphere = (kSubdivision + 1) * (kSubdivision + 1);
	const uint32_t kIndexCountSphere = kSubdivision * kSubdivision * 6;

	// モデルを読み込む
	ModelData modelData = Model::LoadObjFile("Resources", "axis.obj");
	std::unique_ptr<Model> model = std::make_unique<Model>();
	model->Initialize(device, modelData);



	// 球体用のインデックスリソース
	Microsoft::WRL::ComPtr <ID3D12Resource> indexResourceSphere = CreateBufferResource(device, sizeof(uint32_t) * kIndexCountSphere);

	D3D12_INDEX_BUFFER_VIEW indexBufferViewSphere{};
	indexBufferViewSphere.BufferLocation = indexResourceSphere->GetGPUVirtualAddress();
	indexBufferViewSphere.SizeInBytes = sizeof(uint32_t) * kIndexCountSphere;
	indexBufferViewSphere.Format = DXGI_FORMAT_R32_UINT;


	// マテリアル用のリソースを作る
	MaterialConstantBuffer materialResourceSprite;
	materialResourceSprite.Initialize(device);

	// ライト
	DirectionalLightConstantBuffer directionalLightResource;
	directionalLightResource.Initialize(device);

	// WVP
	TransformationMatrixConstantBuffer wvpResource; 
	wvpResource.Initialize(device);


	// textureを読んで転送する
	DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");

	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

	Microsoft::WRL::ComPtr <ID3D12Resource> textureResource = CreateTextureResource(device, metadata);

	Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource = UploadTextureData(textureResource, mipImages, device, commandList);


	// 2枚目のtextureを読む
	DirectX::ScratchImage mipImages2 = LoadTexture(modelData.material.textureFilePath);

	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();

	Microsoft::WRL::ComPtr <ID3D12Resource> textureResource2 = CreateTextureResource(device, metadata2);

	Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource2 = UploadTextureData(textureResource2, mipImages2, device, commandList);

	bool useMonsterBall = true;


	// shaderResourceViewを作る
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = srvHeapManager.GetCPUDescriptorHandle(1);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = srvHeapManager.GetGPUDescriptorHandle(1);

	device->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);


	// 2枚目のSRVを作る
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};

	srvDesc2.Format = metadata2.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = srvHeapManager.GetCPUDescriptorHandle(2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = srvHeapManager.GetGPUDescriptorHandle(2);

	device->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);


	std::unique_ptr<Sprite> sprite = std::make_unique<Sprite>();
	sprite->Initialize(device, 640, 360, textureSrvHandleGPU);


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

	// オーディオ用変数
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;

	IXAudio2MasteringVoice* masterVoice;

	hr = XAudio2Create(xAudio2.GetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR);

	hr = xAudio2->CreateMasteringVoice(&masterVoice);

	SoundData soundData1 = SoundLoadWave("Resources/Alarm01.wav");

	SoundPlayWave(xAudio2.Get(), &soundData1);

	// デバックカメラ
	DebugCamera debugCamera;
	debugCamera.Initialize();

	bool isDebugCameraActive_ = false;


	materialResourceSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialResourceSprite->enabledLighting = 1; 
	materialResourceSprite->uvTransform = MakeIdentityMatrix();

	directionalLightResource->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	directionalLightResource->direction = Vector3(0.0f, -1.0f, 1.0f);
	directionalLightResource->intensity = 1.0f;


#ifdef USE_IMGUI

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(device.Get(), swapChain.GetBufferCount(), rtvDesc.Format, srvHeapManager.GetHeap(),
		srvHeapManager.GetCPUDescriptorHandle(0), srvHeapManager.GetGPUDescriptorHandle(0));
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

			DeviceInputUpdate();

			if (key[DIK_1] && !preKey[DIK_1]) {
				isDebugCameraActive_ = !isDebugCameraActive_;
			}

#ifdef USE_IMGUI

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

#endif

			// ゲームの処理
			Vector3 cameraScale = { 1.0f, 1.0f, 1.0f };
			Vector3 cameraRot = { 0.0f, 0.0f, 0.0f };
			Vector3 cameraPos = { 0.0f, 0.0f, -5.0f };

			Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
			//Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			//Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			//Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
			//Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

			Matrix4x4 viewProjectionMatrix;

			if (isDebugCameraActive_) {
				// デバッグカメラ有効時
				debugCamera.Update(); // 更新処理
				viewProjectionMatrix = Multiply(debugCamera.GetViewMatrix(), debugCamera.GetProjectionMatrix());

			}
			else {
				// 通常カメラ使用時
				Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
				Matrix4x4 viewMatrix = Inverse(cameraMatrix);
				Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
				viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);
			}

			// 3. WVP行列への反映
			wvpResource->WVP = Multiply(worldMatrix, viewProjectionMatrix);
			wvpResource->World = worldMatrix;
			
			// Sprite用のWorldViewProjectionMatrixを作る
			Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(kClientWidth), float(kClientHeight), 0.0f, 100.0f);
			sprite->Update(projectionMatrixSprite);


#ifdef USE_IMGUI

			ImGui::Begin("Debug Settings");

			ImGui::DragFloat3("Sprite Position", &sprite->transform.translate.x, 1.0f);
			ImGui::DragFloat3("Sprite Rotation", &sprite->transform.rotate.x, 0.01f);
			ImGui::DragFloat3("Sprite Scale", &sprite->transform.scale.x, 0.01f);

			ImGui::Checkbox("useMonsterBall", &useMonsterBall);


			ImGui::ColorEdit4("Light Color", &directionalLightResource->color.x);

			if (ImGui::DragFloat3("Light Direction", &directionalLightResource->direction.x, 0.01f, -1.0f, 1.0f)) {

				float length = std::sqrt(directionalLightResource->direction.x * directionalLightResource->direction.x +
					directionalLightResource->direction.y * directionalLightResource->direction.y +
					directionalLightResource->direction.z * directionalLightResource->direction.z);

				if (length != 0) {
					directionalLightResource->direction.x /= length;
					directionalLightResource->direction.y /= length;
					directionalLightResource->direction.z /= length;
				}
			}

			ImGui::DragFloat("Intensity", &directionalLightResource->intensity, 0.01f, 0.0f, 10.0f);


			ImGui::DragFloat3("UVTranslate", &sprite->uvTransform.translate.x, 0.01f, -10.0f, 10.0f);
			ImGui::DragFloat3("UVScale", &sprite->uvTransform.scale.x, 0.01f, -10.0f, 10.0f);
			ImGui::SliderAngle("UVRotate", &sprite->uvTransform.rotate.z, -360.0f, 360.0f);

			ImGui::Text("Model Transform");
			ImGui::DragFloat3("Model Scale", &transform.scale.x, 0.01f);
			ImGui::DragFloat3("Model Rotate", &transform.rotate.x, 0.01f);
			ImGui::DragFloat3("Model Translate", &transform.translate.x, 0.1f);


			ImGui::Checkbox("Debug Camera", &isDebugCameraActive_);

			ImGui::End();

			ImGui::Render();

#endif

			// コマンドを積み込んで確定させる
			UINT backBufferIndex = swapChain.GetCurrentBackBufferIndex();

			// バリアの設定
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = swapChain.GetBuffer(backBufferIndex).Get();
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

			commandList->ResourceBarrier(1, &barrier);

			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeapManager.GetCPUDescriptorHandle(0);
			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
			float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

			commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


			// 描画コマンドを積む
			commandList->RSSetViewports(1, &viewport);

			commandList->RSSetScissorRects(1, &scissorRect);

			commandList->SetGraphicsRootSignature(rootSignature.Get());

			commandList->SetPipelineState(graphicsPipelineState.Get());

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite.GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(1, wvpResource.GetGPUVirtualAddress());
			

			ID3D12DescriptorHeap* descriptorHeaps[] = { srvHeapManager.GetHeap() };
			commandList->SetDescriptorHeaps(1, descriptorHeaps);

			// --- 球体の描画 ---
			commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite.GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(1, wvpResource.GetGPUVirtualAddress());

			commandList->SetGraphicsRootConstantBufferView(2, directionalLightResource.GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(3, textureSrvHandleGPU);

			model->Draw(commandList);

			// --- スプライトの描画 ---
			sprite->Draw(commandList, directionalLightResource.GetResource());


#ifdef USE_IMGUI

			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

#endif

			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

			commandList->ResourceBarrier(1, &barrier);

			hr = commandList->Close();
			assert(SUCCEEDED(hr));


			// コマンドをキックする
			ID3D12CommandList* commandLists[] = { commandList.Get() };

			commandQueue->ExecuteCommandLists(1, commandLists);

			swapChain.Present(1, 0);

			dx12Device.WaitForGPU();

			hr = commandAllocator->Reset();
			assert(SUCCEEDED(hr));

			hr = commandList->Reset(commandAllocator.Get(), nullptr);
			assert(SUCCEEDED(hr));

		}
	}

	xAudio2.Reset();
	SoundUnload(&soundData1);


#ifdef USE_IMGUI

	ImGui_ImplWin32_Shutdown();
	ImGui_ImplDX12_Shutdown();
	ImGui::DestroyContext();

#endif

	CoUninitialize();

	CloseWindow(hwnd);

	return 0;

}