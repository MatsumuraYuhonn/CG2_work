#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <array>
#include <cstddef>
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

    enum class DepthWriteMode {
        kEnable,
        kDisable,
        kCount
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
        void Bind(
            ID3D12GraphicsCommandList* commandList,
            BlendMode blendMode,
            DepthWriteMode depthWriteMode);

        // パーティクル用のインスタンシング描画パイプラインをバインドする。
        void BindParticle(
            ID3D12GraphicsCommandList* commandList,
            BlendMode blendMode,
            DepthWriteMode depthWriteMode);

        // ルートシグネチャを取得
        ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }

        // PSOを取得
        ID3D12PipelineState* GetPipelineState(
            BlendMode blendMode,
            DepthWriteMode depthWriteMode) const;

        ID3D12PipelineState* GetInstancedPipelineState(
            BlendMode blendMode) const;

    private:
        static constexpr size_t kBlendModeCount =
            static_cast<size_t>(BlendMode::kCountOfBlendMode);
        static constexpr size_t kDepthWriteModeCount =
            static_cast<size_t>(DepthWriteMode::kCount);

        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
        Microsoft::WRL::ComPtr<ID3D12RootSignature>
            particleRootSignature_ = nullptr;
        std::array<
            std::array<
                Microsoft::WRL::ComPtr<ID3D12PipelineState>,
                kDepthWriteModeCount>,
            kBlendModeCount> graphicsPipelineStates_{};
        std::array<
            std::array<
                Microsoft::WRL::ComPtr<ID3D12PipelineState>,
                kDepthWriteModeCount>,
            kBlendModeCount> particlePipelineStates_{};
    };

}
