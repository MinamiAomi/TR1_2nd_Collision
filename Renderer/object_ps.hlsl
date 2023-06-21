struct Scene {
    float32_t4x4 view_projection;
    float32_t4 light_color;
    float32_t3 light_direction;
    float32_t light_intensity;
};

ConstantBuffer<Scene> scene_ : register(b0);

struct PixelShaderInput {
    float32_t4 position : SV_POSITION;
    float32_t3 normal : NORMAL0;
    float32_t4 color : COLOR0;
};

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(PixelShaderInput input) {
    PixelShaderOutput output;
    float n_dot_l = dot(normalize(input.normal), -scene_.light_direction);
    float cos = pow(n_dot_l * 0.5f + 0.5f, 2.0f);
    output.color.xyz = input.color.xyz * scene_.light_color.xyz * cos * scene_.light_intensity;
    output.color.w = input.color.w;
    return output;
}