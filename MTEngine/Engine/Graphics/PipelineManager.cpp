#include "PipelineManager.h"
#include "MTEngine/Engine/Debug/Logger.h" 
#include <cassert>


namespace MTEngine {

    namespace {

        D3D12_BLEND_DESC CreateBlendDesc(BlendMode blendMode) {
            D3D12_BLEND_DESC blendDesc{};
            D3D12_RENDER_TARGET_BLEND_DESC& renderTarget =
                blendDesc.RenderTarget[0];

            renderTarget.BlendEnable = FALSE;
            renderTarget.LogicOpEnable = FALSE;
            renderTarget.SrcBlend = D3D12_BLEND_ONE;
            renderTarget.DestBlend = D3D12_BLEND_ZERO;
            renderTarget.BlendOp = D3D12_BLEND_OP_ADD;

            // Sceneビューの描画結果が透明にならないよう、出力先のalphaを維持する。
            renderTarget.SrcBlendAlpha = D3D12_BLEND_ZERO;
            renderTarget.DestBlendAlpha = D3D12_BLEND_ONE;
            renderTarget.BlendOpAlpha = D3D12_BLEND_OP_ADD;
            renderTarget.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            switch (blendMode) {
            case BlendMode::kBlendModeNone:
                break;
            case BlendMode::kBlendModeNormal:
                renderTarget.BlendEnable = TRUE;
                renderTarget.SrcBlend = D3D12_BLEND_SRC_ALPHA;
                renderTarget.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
                break;
            case BlendMode::kBlendModeAdd:
                renderTarget.BlendEnable = TRUE;
                renderTarget.SrcBlend = D3D12_BLEND_SRC_ALPHA;
                renderTarget.DestBlend = D3D12_BLEND_ONE;
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
                break;
            case BlendMode::kBlendModeScreen:
                renderTarget.BlendEnable = TRUE;
                renderTarget.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
                renderTarget.DestBlend = D3D12_BLEND_ONE;
                break;
            default:
                assert(false);
                break;
            }

            return blendDesc;
        }

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
        D3D12_ROOT_PARAMETER rootParameters[5] = {};
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

        // インスタンスごとの変換行列を格納するStructuredBuffer。通常描画では未使用。
        rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[4].Descriptor.ShaderRegister = 1; // t1

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

        Microsoft::WRL::ComPtr<IDxcBlob> instancedVertexShaderBlob =
            shaderCompiler->Compile(
                L"MTEngine/Assets/Shaders/Object3dInstanced.VS.hlsl",
                L"vs_6_0");
        assert(instancedVertexShaderBlob != nullptr);

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

        for (size_t blendIndex = 0;
            blendIndex < kBlendModeCount;
            ++blendIndex) {
            const BlendMode blendMode =
                static_cast<BlendMode>(blendIndex);
            graphicsPipelineStateDesc.BlendState = CreateBlendDesc(blendMode);

            for (size_t depthWriteIndex = 0;
                depthWriteIndex < kDepthWriteModeCount;
                ++depthWriteIndex) {
                const DepthWriteMode depthWriteMode =
                    static_cast<DepthWriteMode>(depthWriteIndex);

                depthStencilDesc.DepthWriteMask =
                    D3D12_DEPTH_WRITE_MASK_ALL;
                if (depthWriteMode == DepthWriteMode::kDisable) {
                    depthStencilDesc.DepthWriteMask =
                        D3D12_DEPTH_WRITE_MASK_ZERO;
                }
                graphicsPipelineStateDesc.DepthStencilState =
                    depthStencilDesc;

                hr = device->CreateGraphicsPipelineState(
                    &graphicsPipelineStateDesc,
                    IID_PPV_ARGS(
                        &graphicsPipelineStates_
                            [blendIndex][depthWriteIndex]));
                assert(SUCCEEDED(hr));
            }
        }

        // パーティクルはインスタンスデータを頂点入力のスロット1から受け取る。
        D3D12_DESCRIPTOR_RANGE particleDescriptorRange{};
        particleDescriptorRange.BaseShaderRegister = 0;
        particleDescriptorRange.NumDescriptors = 1;
        particleDescriptorRange.RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        particleDescriptorRange.OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER particleRootParameters[2]{};
        particleRootParameters[0].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_CBV;
        particleRootParameters[0].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_VERTEX;
        particleRootParameters[0].Descriptor.ShaderRegister = 0;
        particleRootParameters[1].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        particleRootParameters[1].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_PIXEL;
        particleRootParameters[1].DescriptorTable.pDescriptorRanges =
            &particleDescriptorRange;
        particleRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;

        D3D12_ROOT_SIGNATURE_DESC particleRootSignatureDesc{};
        particleRootSignatureDesc.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        particleRootSignatureDesc.pParameters = particleRootParameters;
        particleRootSignatureDesc.NumParameters =
            _countof(particleRootParameters);
        particleRootSignatureDesc.pStaticSamplers = staticSamplers;
        particleRootSignatureDesc.NumStaticSamplers =
            _countof(staticSamplers);

        signatureBlob = nullptr;
        errorBlob = nullptr;
        hr = D3D12SerializeRootSignature(
            &particleRootSignatureDesc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            &signatureBlob,
            &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) {
                Log(reinterpret_cast<char*>(
                    errorBlob->GetBufferPointer()));
            }
            assert(false);
        }
        hr = device->CreateRootSignature(
            0,
            signatureBlob->GetBufferPointer(),
            signatureBlob->GetBufferSize(),
            IID_PPV_ARGS(&particleRootSignature_));
        assert(SUCCEEDED(hr));

        D3D12_INPUT_ELEMENT_DESC particleInputElements[7]{};
        particleInputElements[0] = {
            "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        particleInputElements[1] = {
            "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        for (UINT row = 0; row < 4; ++row) {
            particleInputElements[2 + row] = {
                "INSTANCEWORLD",
                row,
                DXGI_FORMAT_R32G32B32A32_FLOAT,
                1,
                row * sizeof(float) * 4,
                D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
                1 };
        }
        particleInputElements[6] = {
            "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 };

        const D3D12_INPUT_LAYOUT_DESC particleInputLayout = {
            particleInputElements,
            _countof(particleInputElements) };
        const Microsoft::WRL::ComPtr<IDxcBlob> particleVertexShader =
            shaderCompiler->Compile(
                L"MTEngine/Assets/Shaders/Particle.VS.hlsl",
                L"vs_6_0");
        const Microsoft::WRL::ComPtr<IDxcBlob> particlePixelShader =
            shaderCompiler->Compile(
                L"MTEngine/Assets/Shaders/Particle.PS.hlsl",
                L"ps_6_0");
        assert(particleVertexShader != nullptr);
        assert(particlePixelShader != nullptr);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC particlePipelineDesc{};
        particlePipelineDesc.pRootSignature = particleRootSignature_.Get();
        particlePipelineDesc.InputLayout = particleInputLayout;
        particlePipelineDesc.VS = {
            particleVertexShader->GetBufferPointer(),
            particleVertexShader->GetBufferSize() };
        particlePipelineDesc.PS = {
            particlePixelShader->GetBufferPointer(),
            particlePixelShader->GetBufferSize() };
        particlePipelineDesc.RasterizerState = rasterizerDesc;
        particlePipelineDesc.NumRenderTargets = 1;
        particlePipelineDesc.RTVFormats[0] =
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        particlePipelineDesc.PrimitiveTopologyType =
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        particlePipelineDesc.SampleDesc.Count = 1;
        particlePipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        particlePipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        for (size_t blendIndex = 0;
            blendIndex < kBlendModeCount;
            ++blendIndex) {
            const BlendMode blendMode =
                static_cast<BlendMode>(blendIndex);
            particlePipelineDesc.BlendState = CreateBlendDesc(blendMode);

            for (size_t depthWriteIndex = 0;
                depthWriteIndex < kDepthWriteModeCount;
                ++depthWriteIndex) {
                const DepthWriteMode depthWriteMode =
                    static_cast<DepthWriteMode>(depthWriteIndex);
                depthStencilDesc.DepthWriteMask =
                    D3D12_DEPTH_WRITE_MASK_ALL;
                if (depthWriteMode == DepthWriteMode::kDisable) {
                    depthStencilDesc.DepthWriteMask =
                        D3D12_DEPTH_WRITE_MASK_ZERO;
                }
                particlePipelineDesc.DepthStencilState = depthStencilDesc;
                hr = device->CreateGraphicsPipelineState(
                    &particlePipelineDesc,
                    IID_PPV_ARGS(&particlePipelineStates_
                        [blendIndex][depthWriteIndex]));
                assert(SUCCEEDED(hr));
            }
        }
    }

    void PipelineManager::Bind(
        ID3D12GraphicsCommandList* commandList,
        BlendMode blendMode,
        DepthWriteMode depthWriteMode) {
        assert(commandList != nullptr);
        const size_t blendIndex = static_cast<size_t>(blendMode);
        const size_t depthWriteIndex =
            static_cast<size_t>(depthWriteMode);
        assert(blendIndex < kBlendModeCount);
        assert(depthWriteIndex < kDepthWriteModeCount);

        commandList->SetGraphicsRootSignature(rootSignature_.Get());
        commandList->SetPipelineState(
            graphicsPipelineStates_
                [blendIndex][depthWriteIndex].Get());
    }

    void PipelineManager::BindParticle(
        ID3D12GraphicsCommandList* commandList,
        BlendMode blendMode,
        DepthWriteMode depthWriteMode) {
        assert(commandList != nullptr);
        const size_t blendIndex = static_cast<size_t>(blendMode);
        const size_t depthWriteIndex =
            static_cast<size_t>(depthWriteMode);
        assert(blendIndex < kBlendModeCount);
        assert(depthWriteIndex < kDepthWriteModeCount);

        commandList->SetGraphicsRootSignature(
            particleRootSignature_.Get());
        commandList->SetPipelineState(
            particlePipelineStates_
                [blendIndex][depthWriteIndex].Get());
    }

    ID3D12PipelineState* PipelineManager::GetPipelineState(
        BlendMode blendMode,
        DepthWriteMode depthWriteMode) const {
        const size_t blendIndex = static_cast<size_t>(blendMode);
        const size_t depthWriteIndex =
            static_cast<size_t>(depthWriteMode);
        assert(blendIndex < kBlendModeCount);
        assert(depthWriteIndex < kDepthWriteModeCount);
        return graphicsPipelineStates_
            [blendIndex][depthWriteIndex].Get();
    }

    ID3D12PipelineState* PipelineManager::GetInstancedPipelineState(
        BlendMode blendMode) const {
        const size_t index = static_cast<size_t>(blendMode);
        assert(index < kBlendModeCount);
        return instancedGraphicsPipelineStates_[index].Get();
    }

}
