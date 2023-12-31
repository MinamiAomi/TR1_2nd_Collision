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
    void DrawCapsule(const Matrix4x4& world_matrix, const Vector4& color, DrawMode draw_mode);
    
    void DrawObject(std::size_t mesh_handle, const Vector3& scale, const Quaternion& rotate, const Vector3& translate, const Vector4& color);

private:
    Renderer(const Renderer&) = delete;
    const Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void CreateBoxMesh();
    void CreateSphereMesh();
    void CreateCapusleMesh();

    class Impl; Impl* const pimpl_;

    std::size_t plane_;
    std::size_t box_;
    std::size_t sphere_;
};