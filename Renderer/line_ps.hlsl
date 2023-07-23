struct Scene {
    float32_t4x4 viewProjection;
};

ConstantBuffer<Scene> scene_ : register(b0);

struct PixelShaderInput {
    float32_t4 position : SV_POSITION;
    float32_t4 color : COLOR0;
};

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(PixelShaderInput input) {
    PixelShaderOutput output;
    output.color = input.color;
    return output;
}