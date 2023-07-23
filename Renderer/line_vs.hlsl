struct Scene {
    float32_t4x4 viewProjection;
};

ConstantBuffer<Scene> scene_ : register(b0);

struct VertexShaderOutput {
    float32_t4 position : SV_POSITION;
    float32_t4 color : COLOR0;
};

struct VertexShaderInput {
    float32_t3 position : POSITION0;
    float32_t4 color : COLOR0;
};

VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput output;
    output.position = mul(float32_t4(input.position, 1.0f), scene_.viewProjection);
    output.color = input.color;
    return output;
}