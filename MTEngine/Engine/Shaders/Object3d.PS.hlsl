#include "Object3d.hlsli"

// 方向性光源の構造体
    struct DirectionalLight
    {
        float4 color;
        float3 direction;
        float intensity;
    };

// 定数バッファの定義
    ConstantBuffer<Material> gMaterial : register(b0);
    ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2);

// テクスチャとサンプラー
    Texture2D<float4> gTexture : register(t0);
    SamplerState gSampler : register(s0);

// 出力構造体
    struct PixelShaderOutput
    {
        float4 color : SV_TARGET0;
    };

// ピクセルシェーダーメイン処理
    PixelShaderOutput main(VertexShaderOutput input)
    {
        PixelShaderOutput output;
    
    // テクスチャのサンプリング
        float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    // ライティング計算の切り替え
        if (gMaterial.enabledLighting != 0)
        {
        // HalfLambertライティング：影の境界を柔らかくする手法
            float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        
            output.color = gMaterial.color * textureColor * gDirectionalLight.color * cos * gDirectionalLight.intensity;
        }
        else
        {
        // ライティング無効時はテクスチャカラーとマテリアルカラーのみ
            output.color = gMaterial.color * textureColor;
        }
    
        return output;
    }