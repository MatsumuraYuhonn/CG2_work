#include "object3d.hlsli"

// 変換行列用定数バッファ
    struct TransformationMatrix
    {
        float4x4 WVP;
        float4x4 World;
    };

    ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);
    ConstantBuffer<Material> gMaterial : register(b0);

// 入力頂点データ
    struct VertexShaderInput
    {
        float4 position : POSITION;
        float2 texcoord : TEXCOORD;
        float3 normal : NORMAL;
    };

// 頂点シェーダーメイン処理
    VertexShaderOutput main(VertexShaderInput input)
    {
        VertexShaderOutput output;
    
    // 位置の変換（WVP行列を使用）
        output.position = mul(input.position, gTransformationMatrix.WVP);
    
    // UVの変換
        output.texcoord = TransformUV(input.texcoord, gMaterial.uvTransform);
    
    // 法線の変換（ワールド行列の3x3部分を使用）
        output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.World));
    
        return output;
    }