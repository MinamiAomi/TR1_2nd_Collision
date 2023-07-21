#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Math/MathUtils.hpp"

enum class DrawMode {
    kObject,
    kWireFrame
};

class Renderer {
public:
    static const std::uint32_t kMaxObjectCount = 1024;

    Renderer();
    ~Renderer();

    void Initailize(const std::wstring& window_title, const std::uint32_t window_width, const std::uint32_t window_height);
    void StartRendering();
    void EndRendering();
    void Finalize();

    std::size_t RegisterMesh(const std::vector<Vector3>& positions, const std::vector<std::uint16_t> indices);

    void DrawPlane(const Matrix4x4& world_matrix, const Vector4& color, DrawMode draw_mode);
    void DrawBox(const Matrix4x4& world_matrix, const Vector4& color, DrawMode draw_mode);
    void DrawSphere(const Matrix4x4& world_matrix, const Vector4& color, DrawMode draw_mode);
    void DrawCylinder(const Matrix4x4& world_matrix, const Vector4& color, DrawMode draw_mode);

    void DrawBox(const Vector3& center, const Vector3& size, const Matrix4x4& worldMatrix, const Vector4& color);
    void DrawSphere(const Vector3& center, float radius, const Matrix4x4& worldMatrix, const Vector4& color);

    void DrawObject(std::size_t mesh_handle, const Vector3& scale, const Quaternion& rotate, const Vector3& translate, const Vector4& color);

    void SetCamera(const Vector3& pos, const Vector3& rot);
    void SetLight(const Vector3& direction, const Vector4& color, float intensity);

private:
    Renderer(const Renderer&) = delete;
    const Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    std::size_t CreatePlaneMesh();
    std::size_t CreateBoxMesh();
    std::size_t CreateSphereMesh();
    std::size_t CreateCylinderMesh();

    class Impl; Impl* const pimpl_;

    std::size_t plane_ = 0;
    std::size_t box_ = 0;
    std::size_t sphere_ = 0;
    std::size_t cylinder_ = 0;
};

#include <array>
#include <cstdint>
#include <vector>
#include <unordered_map>

#include "DX12/DX12.h"

class Renderer_ {
public:
    struct Vertex {
        Vector3 position;
        Vector3 normal;
    };

    struct MeshHandle {
        size_t handle;
    };

    void Initialize();
    void StartRendering();
    void EndRendering();
    void Finalize();

    MeshHandle RegisterMesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);

    void DrawBox();
    void DrawSphere();
    void DrawCylinder();
    void DrawCone();

    void DrawMesh(MeshHandle meshHandle);

    void SetCamera(const Vector3& position, const Quaternion& rotate);
    void SetDirectionalLight(const Vector3& direction, const Vector4& color, float intensity);

private:
    static constexpr size_t kMaxMeshCount = 64;
    static constexpr size_t kMaxObjectCount = 256;
    static constexpr size_t kMaxLineCount = 16384;

    struct Scene {
        Matrix4x4 viewProjectionMatrix;
        Vector4 lightColor;
        Vector3 lightDirection;
        float intensity;
    };
    struct Mesh {
        CG::DX12::Resource vertexBuffer;
        CG::DX12::VertexBufferView vertexBufferView;
        CG::DX12::Resource indexBuffer;
        CG::DX12::IndexBufferView IndexBufferView;
    };
    struct Instance {
        Vector4 mat0;
        Vector4 mat1;
        Vector4 mat2;
        Vector4 color;
    };

    void InitializeWin();
    void InitializeDX12Core();
    void InitializePSO();
    void InitializeBuffers();

    HWND hWnd_{ nullptr };
    uint32_t clientWidth_{ 1280 };
    uint32_t clientHeight_{ 720 };

    std::unique_ptr<CG::DX12::Device> device_;

    std::unique_ptr<CG::DX12::CommandQueue> commandQueue_;
    std::unique_ptr<CG::DX12::CommandList> commandList_;
    std::unique_ptr<CG::DX12::Fence> fence_;

    std::unique_ptr<CG::DX12::DescriptorHeap> rtvDescriptorHeap_;
    std::unique_ptr<CG::DX12::DescriptorHeap> dsvDescriptorHeap_;
    std::unique_ptr<CG::DX12::DescriptorHeap> srvDescriptorHeap_;

    std::unique_ptr<CG::DX12::SwapChain> swapChain_;
    std::unique_ptr<CG::DX12::DepthStencilResource> depthStencilResource_;

    std::unique_ptr<CG::DX12::RootSignature> rootSignature_;
    std::unique_ptr<CG::DX12::PipelineState> objectPipelineState_;
    std::unique_ptr<CG::DX12::PipelineState> wireFramePipelineState_;

    std::array<std::unique_ptr<Mesh>, kMaxMeshCount> meshes_;
    std::unique_ptr<CG::DX12::DynamicBuffer> instancingBuffer_;
    std::unordered_map<size_t, std::vector<Instance>> objectInstancesMap;
    std::unordered_map<size_t, std::vector<Instance>> wireFrameInstancesMap;
    size_t objectCounter_{ 0 };

    std::unique_ptr<CG::DX12::PipelineState> linePipelineState_;
    std::unique_ptr<CG::DX12::DynamicBuffer> lineBuffer;
    size_t lineCounter_{ 0 };

    std::unique_ptr<CG::DX12::DynamicBuffer> sceneConstantBuffer_;
    Scene scene_{};
};