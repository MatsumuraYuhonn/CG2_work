struct TranceformationMatrix {
    
    float4x4 WVP;
    
};

ConstantBuffer<TranceformationMatrix> gTranceformationMatrix : register(b0);

struct VertexShaderOutput {
    
    float4 position : SV_POSITION;
};

struct VertexShaderInput {
    
    float4 position : POSITION0;
};

VertexShaderOutput main(VertexShaderInput input) {
    
    VertexShaderOutput output;
    output.position = mul(input.position, gTranceformationMatrix.WVP);
    
    return output;
    
}
