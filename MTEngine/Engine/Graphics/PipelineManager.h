#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <array>
#include <cassert>
#include "MTEngine/Engine/Base/ShaderCompiler.h"

// グラフィックスパイプラインステート(PSO)およびルートシグネチャの生成・管理を行うクラス
namespace MTEngine {

    enum class BlendMode {
        kBlendModeNone,
        kBlendModeNormal,
        kBlendModeAdd,
        kBlendModeSubtract,
        kBlendModeMultiply,
        kBlendModeScreen,
        kCountOfBlendMode
    };

    class PipelineManager {
    public:
        PipelineManager() = default;
        ~PipelineManager() = default;

        // 初期化処理：ルートシグネチャとPSOの生成を行う
        // device: D3D12デバイス
        // shaderCompiler: シェーダーコンパイル用クラス
        void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, ShaderCompiler* shaderCompiler);

        // コマンドリストにパイプライン状態をバインドする
        // commandList: 使用するグラフィックスコマンドリスト
        void Bind(ID3D12GraphicsCommandList* commandList, BlendMode blendMode);

        // ルートシグネチャを取得
        ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }

        // PSOを取得
        ID3D12PipelineState* GetPipelineState(BlendMode blendMode) const {

            const size_t index = static_cast<size_t>(blendMode);

            return  graphicsPipelineStates_[index].Get();

        }

    private:
        static constexpr size_t kBlendModeCount = static_cast<size_t>(BlendMode::kCountOfBlendMode);

        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
       
		std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, kBlendModeCount> graphicsPipelineStates_ = {};
       
    };

}