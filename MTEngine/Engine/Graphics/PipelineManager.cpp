#include "PipelineManager.h"
#include "MTEngine/Engine/Debug/Logger.h" 
#include <cassert>


namespace MTEngine {

	D3D12_BLEND_DESC CreateBlendDesc(BlendMode blendMode) {

		D3D12_BLEND_DESC blendDesc{};

		D3D12_RENDER_TARGET_BLEND_DESC& renderTarget = blendDesc.RenderTarget[0];

		// ブレンドなしの場合にも使用できる基本設定
		renderTarget.BlendEnable = FALSE;
		renderTarget.LogicOpEnable = FALSE;
		renderTarget.SrcBlend = D3D12_BLEND_ONE;
		renderTarget.DestBlend = D3D12_BLEND_ZERO;
		renderTarget.BlendOp = D3D12_BLEND_OP_ADD;

		// 出力先のアルファ値を維持する
		renderTarget.SrcBlendAlpha = D3D12_BLEND_ZERO;

		renderTarget.DestBlendAlpha = D3D12_BLEND_ONE;

		renderTarget.BlendOpAlpha = D3D12_BLEND_OP_ADD;

		renderTarget.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		switch (blendMode) {
		case BlendMode::kBlendModeNone:
			renderTarget.BlendEnable = FALSE;
			break;

		case BlendMode::kBlendModeNormal:
			renderTarget.BlendEnable = TRUE;
			renderTarget.SrcBlend = D3D12_BLEND_SRC_ALPHA;

			renderTarget.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			break;

		case BlendMode::kBlendModeAdd:
			renderTarget.BlendEnable = TRUE;
			renderTarget.SrcBlend = D3D12_BLEND_SRC_ALPHA;

			renderTarget.DestBlend = D3D12_BLEND_ONE;

			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			break;

		case BlendMode::kBlendModeSubtract:
			renderTarget.BlendEnable = TRUE;
			renderTarget.SrcBlend = D3D12_BLEND_SRC_ALPHA;

			renderTarget.DestBlend = D3D12_BLEND_ONE;

			renderTarget.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
			break;

		case BlendMode::kBlendModeMultiply:
			renderTarget.BlendEnable = TRUE;
			renderTarget.SrcBlend = D3D12_BLEND_ZERO;

			renderTarget.DestBlend = D3D12_BLEND_SRC_COLOR;

			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			break;

		case BlendMode::kBlendModeScreen:
			renderTarget.BlendEnable = TRUE;
			renderTarget.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;

			renderTarget.DestBlend = D3D12_BLEND_ONE;

			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			break;

		default:
			assert(false);
			break;
		}

		return blendDesc;
	}


	void PipelineManager::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, ShaderCompiler* shaderCompiler) {
		HRESULT hr = S_OK;

		// 1. ルートシグネチャの作成 (main.cpp から完全移植)
		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
		descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// DescriptorRange の設定 (rootParameters[3] で使用)
		D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
		descriptorRange[0].BaseShaderRegister = 0;
		descriptorRange[0].NumDescriptors = 1;
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// ルートパラメータの設定
		D3D12_ROOT_PARAMETER rootParameters[4] = {};
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[1].Descriptor.ShaderRegister = 1;

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[2].Descriptor.ShaderRegister = 2;

		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[3].DescriptorTable.pDescriptorRanges = descriptorRange;
		rootParameters[3].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // t0

		descriptionRootSignature.pParameters = rootParameters;
		descriptionRootSignature.NumParameters = _countof(rootParameters);

		// スタティックサンプラーの設定
		D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
		staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
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
			if (errorBlob) {
				Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
				OutputDebugStringA(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
			}
			assert(false);
		}

		hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
		assert(SUCCEEDED(hr));


		// 2. インプットレイアウトの設定 (main.cpp から完全移植)
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

		D3D12_RASTERIZER_DESC rasterizerDesc{};
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;


		// 3. シェーダーのコンパイル (引数の shaderCompiler を使用)
		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = shaderCompiler->Compile(L"MTEngine/Assets/Shaders/object3d.VS.hlsl", L"vs_6_0");
		assert(vertexShaderBlob != nullptr);

		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = shaderCompiler->Compile(L"MTEngine/Assets/Shaders/object3d.PS.hlsl", L"ps_6_0");
		assert(pixelShaderBlob != nullptr);


		// 4. PSOの生成
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
		graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();
		graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
		graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
		graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
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


		// 5. ブレンドモードごとにPSOを生成
		for (size_t index = 0; index < kBlendModeCount; ++index) {
			const BlendMode blendMode = static_cast<BlendMode>(index);

			// ブレンドステートを設定
			graphicsPipelineStateDesc.BlendState =
				CreateBlendDesc(blendMode);

			// PSOを生成
			hr = device->CreateGraphicsPipelineState(
				&graphicsPipelineStateDesc,
				IID_PPV_ARGS(&graphicsPipelineStates_[index])
			);

		}

		assert(SUCCEEDED(hr));
	}


	void PipelineManager::Bind(
		ID3D12GraphicsCommandList* commandList,
		BlendMode blendMode) {

		assert(commandList != nullptr);

		const size_t blendModeIndex =
			static_cast<size_t>(blendMode);

		assert(blendModeIndex < kBlendModeCount);

		commandList->SetGraphicsRootSignature(
			rootSignature_.Get());

		commandList->SetPipelineState(
			graphicsPipelineStates_[blendModeIndex].Get());
	}

}
