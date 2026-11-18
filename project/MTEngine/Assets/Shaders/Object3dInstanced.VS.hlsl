#include "object3d.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
};

StructuredBuffer<TransformationMatrix> gTransformationMatrices : register(t1);
ConstantBuffer<Material> gMaterial : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

VertexShaderOutput main(
    VertexShaderInput input,
    uint instanceId : SV_InstanceID)
{
    const TransformationMatrix transformation =
        gTransformationMatrices[instanceId];

    VertexShaderOutput output;
    output.position = mul(input.position, transformation.WVP);
    output.texcoord = TransformUV(input.texcoord, gMaterial.uvTransform);
    output.normal = normalize(
        mul(input.normal, (float3x3) transformation.World));

    return output;
}
