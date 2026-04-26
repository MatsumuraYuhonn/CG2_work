#include "object3d.hlsli"

struct TranceformationMatrix {
    
    float4x4 WVP;
    
};

ConstantBuffer<TranceformationMatrix> gTranceformationMatrix : register(b0);

struct VertexShaderInput {
    
    float4 position : POSITION0;
    
    float2 texcoord : TEXCOORD0;
};

VertexShaderOutput main(VertexShaderInput input) {
    
    VertexShaderOutput output;
    
    output.position = mul(input.position, gTranceformationMatrix.WVP);
    
    output.texcoord = input.texcoord;
    
    return output;
    
}
