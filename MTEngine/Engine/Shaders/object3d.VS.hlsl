#include "object3d.hlsli"

struct TransformationMatrix {
    
    float4x4 WVP;
    
    float4x4 World;
    
};


ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

ConstantBuffer<Material> gMaterial : register(b0); 

struct VertexShaderInput
{
    
    float4 position : POSITION;
    
    float2 texcoord : TEXCOORD;
    
    float3 normal : NORMAL;
    
};

VertexShaderOutput main(VertexShaderInput input) {
    
    VertexShaderOutput output;
    
    output.position = mul(input.position, gTransformationMatrix.WVP);
    
    output.texcoord = TransformUV(input.texcoord, gMaterial.uvTransform);
    
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.World));
    
    return output;
    
}
