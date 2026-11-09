struct ParticleCamera
{
    float4x4 viewProjection;
};

ConstantBuffer<ParticleCamera> gParticleCamera : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD;
    float4 world0 : INSTANCEWORLD0;
    float4 world1 : INSTANCEWORLD1;
    float4 world2 : INSTANCEWORLD2;
    float4 world3 : INSTANCEWORLD3;
    float4 color : COLOR0;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    const float4x4 world = float4x4(
        input.world0,
        input.world1,
        input.world2,
        input.world3);
    const float4 worldPosition = mul(input.position, world);
    output.position = mul(worldPosition, gParticleCamera.viewProjection);
    output.texcoord = input.texcoord;
    output.color = input.color;
    return output;
}
