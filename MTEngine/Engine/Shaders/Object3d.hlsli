struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    
    float2 texcoord : TEXCOORD0;
    
    float3 normal : NORMAL0;
    
};


struct Material
{
    float4 color;
    int enabledLighting;
    float4x4 uvTransform;
};


float2 TransformUV(float2 texcoord, float4x4 transform)
{
    float4 transformedUV = mul(float4(texcoord,0.0f, 1.0f), transform);
    return transformedUV.xy;
}
