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


// カメラの構造体
struct Camera
{
    float3 worldPosition;
    float padding;
};

ConstantBuffer<Camera> gCamera : register(b3);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 textureColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (gMaterial.useTexture != 0)
    {
        textureColor = gTexture.Sample(gSampler, input.texcoord);
    }

    // ライティングが有効、かつモードがNone（0）でない場合にライティング計算を行う
    if (gMaterial.enabledLighting != 0 && gMaterial.lightingMode != 0)
    {
        
        float3 normal = normalize(input.normal);

        float3 lightDirection =
        normalize(-gDirectionalLight.direction);

        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = 0.0f;
        
        // ライティングモードによる切り替え
        if (gMaterial.lightingMode == 3)
        {
        
            // Phongライティング計算
            float diffuseFactor = saturate(NdotL);
            
            // ビュー方向の計算
            float3 diffuseColor = gMaterial.color.rgb * textureColor.rgb *
            gDirectionalLight.color.rgb * diffuseFactor * gDirectionalLight.intensity;
        
            // 反射方向の計算
            float3 viewDirection = normalize(gCamera.worldPosition - input.worldPosition);
            
            // 反射光の計算
            float3 reflectedLight = reflect(-lightDirection, normal);
            
            // 鏡面反射の強さ
            float specularFactor = 0.0f;
            
            if (diffuseFactor > 0.0f)
            {
                
                float RdotV = saturate(dot(reflectedLight, viewDirection));
                
                specularFactor = pow(RdotV, gMaterial.shininess);
                
            }
            
            // 鏡面反射の色
            float3 specularColor = gDirectionalLight.color.rgb * specularFactor * gDirectionalLight.intensity;
            
            // 最終的な色の計算
            output.color.rgb = diffuseColor + specularColor;
            
            output.color.a = gMaterial.color.a * textureColor.a;
            
        }
        else
        {
            // Phong以外で使用する明るさ
            float cos = 0.0f;

            if (gMaterial.lightingMode == 2)
            {
            // Half-Lambert
                cos = pow(
                NdotL * 0.5f + 0.5f,
                2.0f);
            }
            else
            {
            // Lambert
                cos = max(0.0f, NdotL);
            }

            // Lambert・Half-Lambertの最終色
            output.color.rgb =
            gMaterial.color.rgb *
            textureColor.rgb *
            gDirectionalLight.color.rgb *
            cos *
            gDirectionalLight.intensity;

            output.color.a =
            gMaterial.color.a *
            textureColor.a;
        }
        
    }
    else
    {
        // ライティングなし
        output.color =
        gMaterial.color * textureColor;
    }

    if (gMaterial.isSelected != 0)
    {
        output.color.rgb = saturate(output.color.rgb * 1.35f + 0.12f);
    }
    
    
    
    return output;
}
