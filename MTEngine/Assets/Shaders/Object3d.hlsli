// 頂点シェーダーからピクセルシェーダーへ渡すデータ
struct VertexShaderOutput
{
    float4 position : SV_POSITION; // クリップ空間での頂点位置
    float2 texcoord : TEXCOORD0; // UV座標
    float3 normal : NORMAL0; // 法線ベクトル
};

// マテリアル定数バッファ（C++側のMaterial構造体と一致させる）
struct Material
{
    float4 color; // スプライトの色
    int enabledLighting; // ライティングの有効フラグ
    float4x4 uvTransform; // UV変換用行列
    int lightingMode;

};
    
float2 TransformUV(float2 texcoord, float4x4 transform)
{
    float4 transformedUV = mul(float4(texcoord, 0.0f, 1.0f), transform);
    return transformedUV.xy;
}