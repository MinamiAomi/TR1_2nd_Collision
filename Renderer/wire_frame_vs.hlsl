struct Scene {
    float32_t4x4 view_projection;
};

ConstantBuffer<Scene> scene_ : register(b0);

struct VertexShaderOutput {
    float32_t4 position : SV_POSITION;
    float32_t3 normal : NORMAL0;
    float32_t4 color : COLOR0;
};

struct VertexShaderInput {
    float32_t3 position : POSITION0;
    float32_t3 normal : NORMAL0;

    float32_t3x4 world_matrix : MATRIX;
    float32_t4 color : COLOR0;
};

VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput output;
    float32_t3 world_pos = mul(input.world_matrix, float32_t4(input.position, 1.0f));
    output.position = mul(float32_t4(world_pos, 1.0f), scene_.view_projection);
    output.normal = float32_t3(0.0f, 0.0f, 0.0f);
    output.color = input.color;
    return output;
}