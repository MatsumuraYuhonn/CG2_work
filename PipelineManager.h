#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include "ShaderCompiler.h"

class PipelineManager {
public:
    PipelineManager() = default;
    ~PipelineManager() = default;

    // 初期化: ルートシグネチャとPSOの生成
    void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, ShaderCompiler* shaderCompiler);

    // コマンドリストへの適用
    void Bind(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

    // 必要に応じてゲッターも用意
    ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
    ID3D12PipelineState* GetPipelineState() const { return graphicsPipelineState_.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;
};