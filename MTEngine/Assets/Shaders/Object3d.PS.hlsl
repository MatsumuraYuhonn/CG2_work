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


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    // ライティングが有効、かつモードがNone（0）でない場合にライティング計算を行う
    if (gMaterial.enabledLighting != 0 && gMaterial.lightingMode != 0)
    {
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = 0.0f;

        // ライティングモードによる切り替え
        if (gMaterial.lightingMode == 2) // "Half-Lambert"
        {
            cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        }
        else // "Lambert" (mode == 1)
        {
            cos = max(0.0f, NdotL);
        }
        
        output.color = gMaterial.color * textureColor * gDirectionalLight.color * cos * gDirectionalLight.intensity;
    }
    else
    {
        // ライティングなし、または明示的にNoneが選ばれている場合
        output.color = gMaterial.color * textureColor;
    }
    
    return output;
}