#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Math/MathUtils.hpp"

struct Vertex {
    Vector3 position;
    Vector3 normal;
};

enum class DrawMode {
    kObject,
    kWireFrame
};

struct Box {
    Vector3 size;
    Quaternion rotate;
    Vector3 translate;
};

struct Sphere {
    Vector3 center;
    float radius;
};

struct Capsule {
    Vector3 center;
    float radius;
    float height;
    Vector3 direction;
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

    std::size_t RegisterMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint16_t> indices);

    void DrawBox(const Box& box, const Vector4& color);
    void DrawSphere(const Sphere& sphere, const Vector4& color);
    void DrawCapsule(const Capsule& cspusle, const Vector4& color);
    
    void DrawObject(std::size_t mesh_handle, const Vector3& scale, const Quaternion& rotate, const Vector3& translate, const Vector4& color);

private:
    Renderer(const Renderer&) = delete;
    const Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    class Impl; Impl* const pimpl_;
};