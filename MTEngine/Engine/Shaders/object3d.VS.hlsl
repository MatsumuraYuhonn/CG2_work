#include "object3d.hlsli"

struct TranceformationMatrix {
    
    float4x4 WVP;
    
    float4x4 World;
    
};


ConstantBuffer<TranceformationMatrix> gTranceformationMatrix : register(b1);

ConstantBuffer<Material> gMaterial : register(b0); 

struct VertexShaderInput
{
    
    float4 position : POSITION;
    
    float2 texcoord : TEXCOORD;
    
    float3 normal : NORMAL;
    
};

VertexShaderOutput main(VertexShaderInput input) {
    
    VertexShaderOutput output;
    
    output.position = mul(input.position, gTranceformationMatrix.WVP);
    
    output.texcoord = TransformUV(input.texcoord, gMaterial.uvTransform);
    
    output.normal = normalize(mul(input.normal, (float3x3) gTranceformationMatrix.World));
    
    return output;
    
}
