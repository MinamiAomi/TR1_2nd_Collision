#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <unordered_map>

#include "DX12/DX12.h"
#include "Math/MathUtils.hpp"

class Renderer {
public:
    enum class DrawMode {
        Object,
        WireFrame
    };

    struct Vertex {
        Vector3 position;
        Vector3 normal;
    };

    struct MeshHandle {
        size_t handle;
    };

    void Initialize(uint32_t windowWidth, uint32_t windowHeight);
    void StartRendering();
    void EndRendering();
    void Finalize();

    MeshHandle RegisterMesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);

    void DrawPlane(const Matrix4x4& worldMatrix, const Vector4& color, DrawMode drawMode);
    void DrawBox(const Matrix4x4& worldMatrix, const Vector4& color, DrawMode drawMode);
    void DrawSphere(const Matrix4x4& worldMatrix, const Vector4& color, DrawMode drawMode);
    void DrawCylinder(const Matrix4x4& worldMatrix, const Vector4& color, DrawMode drawMode);
    void DrawCone(const Matrix4x4& worldMatrix, const Vector4& color, DrawMode drawMode);
    void DrawMesh(MeshHandle meshHandle, const Matrix4x4& worldMatrix, const Vector4& color, DrawMode drawMode);

    void DrawLine(const Vector3& v1, const Vector3& v2, const Vector4& color);

    void SetViewProjectionMatrix(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);
    void SetDirectionalLight(const Vector3& direction, const Vector4& color, float intensity);

private:
    static constexpr size_t kMaxMeshCount = 64;
    static constexpr size_t kMaxInstanceCount = 256;
    static constexpr size_t kMaxLineCount = 16384;

    struct Scene {
        Matrix4x4 viewProjectionMatrix;
        Vector4 lightColor;
        Vector3 lightDirection;
        float lightIntensity;
    };
    struct Mesh {
        CG::DX12::Resource vertexBuffer;
        CG::DX12::VertexBufferView vertexBufferView;
        CG::DX12::Resource indexBuffer;
        CG::DX12::IndexBufferView IndexBufferView;
    };
    struct Instance {
        Vector4 worldMatrix[3];
        Vector4 color;
    };
    struct LineVertex {
        Vector3 position;
        Vector4 color;
    };

    void InitializeWin();
    void InitializeDX12Core();
    void InitializePSO();
    void InitializeBuffers();

    void CreateMeshPlane();
    void CreateMeshBox();
    void CreateMeshSphere();
    void CreateMeshCylinder();
    void CreateMeshCone();

    void AddInstance(MeshHandle mesh, const Matrix4x4& worldMatrix, const Vector4& color, DrawMode drawMode);
    void AddLine(const Vector3& p0, const Vector3& p1, const Vector4& color);

    void DrawAllInstances();
    void DrawAllLines();

    HWND hWnd_{ nullptr };
    uint32_t windowWidth_{ 0 };
    uint32_t windowHeight_{ 0 };

    std::unique_ptr<CG::DX12::Device> device_;

    std::unique_ptr<CG::DX12::CommandQueue> commandQueue_;
    std::array<std::unique_ptr<CG::DX12::CommandList>, CG::DX12::SwapChain::kBackBufferCount> commandLists_;
    std::unique_ptr<CG::DX12::Fence> fence_;

    std::unique_ptr<CG::DX12::DescriptorHeap> rtvDescriptorHeap_;
    std::unique_ptr<CG::DX12::DescriptorHeap> dsvDescriptorHeap_;
    std::unique_ptr<CG::DX12::DescriptorHeap> srvDescriptorHeap_;

    std::unique_ptr<CG::DX12::SwapChain> swapChain_;
    std::unique_ptr<CG::DX12::DepthStencilResource> depthStencilResource_;

    D3D12_VIEWPORT viewport_{};
    D3D12_RECT scissorRect_{};

    CG::DX12::Descriptor imguiDescriptor_;

    std::unique_ptr<CG::DX12::RootSignature> rootSignature_;
    std::unique_ptr<CG::DX12::PipelineState> objectPipelineState_;
    std::unique_ptr<CG::DX12::PipelineState> wireFramePipelineState_;

    std::array<std::unique_ptr<Mesh>, kMaxMeshCount> meshes_;
    size_t nextMeshIndex{ 0 };
    std::array<std::unique_ptr<CG::DX12::DynamicBuffer>, CG::DX12::SwapChain::kBackBufferCount>  instancingBuffers_;
    std::unordered_map<size_t, std::vector<Instance>> objectInstancesMap;
    std::unordered_map<size_t, std::vector<Instance>> wireFrameInstancesMap;
    size_t instanceCount_{ 0 };

    std::unique_ptr<CG::DX12::PipelineState> linePipelineState_;
    std::array<std::unique_ptr<CG::DX12::DynamicBuffer>, CG::DX12::SwapChain::kBackBufferCount> lineBuffers_;
    size_t lineCount_{ 0 };

    std::array<std::unique_ptr<CG::DX12::DynamicBuffer>, CG::DX12::SwapChain::kBackBufferCount> sceneBuffers_;
    Scene scene_{};

    MeshHandle plane_{};
    MeshHandle box_{};
    MeshHandle sphere_{};
    MeshHandle cylinder_{};
    MeshHandle cone_{};
};