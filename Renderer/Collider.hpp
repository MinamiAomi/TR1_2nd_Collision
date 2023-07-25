#pragma once

#include <functional>
#include <vector>

#include "Math/MathUtils.hpp"
#include "AABB.hpp"
#include "Transform.hpp"

class Collider;

struct CollisionInfo {
    Collider* collider;
    Vector3 normal;
    float depth;
};

class Collider {
public:

    virtual ~Collider() {}
    virtual Vector3 FindFurthestPoint(const Vector3& direction) const = 0;
    virtual void UpdateAABB() = 0;

    void CallBack(const CollisionInfo& info) { if (callBack_) { callBack_(info); } }

    void SetCallBack(const std::function<void(const CollisionInfo&)>& callBack) { callBack_ = callBack; }

    const AABB& GetAABB() const { return aabb_; }

    Transform transform;
    Vector4 color{1,1,1,1};
    bool isStatic = false;

protected:
    AABB aabb_{};
private:
    std::function<void(const CollisionInfo&)> callBack_;
};

class SphereCollider : public Collider {
public:
    Vector3 center;
    float radius{ 0.5f };

    Vector3 FindFurthestPoint(const Vector3& direction) const override {
        return direction.Normalized() * (radius * transform.scale.Max()) + center * transform.GetWorldMatrix();
    }
    void UpdateAABB() override {
        float size = radius * transform.scale.Max();
        Vector3 offset = center * transform.GetWorldMatrix();
        aabb_.min = Vector3(-size) + offset;
        aabb_.max = Vector3(size) + offset;
    }
};

class BoxCollider : public Collider {
public:
    Vector3 center;
    Vector3 size{ 1.0f };

    Vector3 FindFurthestPoint(const Vector3& direction) const override {
        Vector3 localDirection = transform.GetWorldMatrixInverse().ApplyRotation(direction);
        Vector3 localPoint;
        localPoint.x = (localDirection.x > 0.0f ? size.x : -size.x) * 0.5f;
        localPoint.y = (localDirection.y > 0.0f ? size.y : -size.y) * 0.5f;
        localPoint.z = (localDirection.z > 0.0f ? size.z : -size.z) * 0.5f;

        return (localPoint + center) * transform.GetWorldMatrix();
    }

    void UpdateAABB() override {
        Vector3 halfSize = size * 0.5f;
        Vector3 vertices[] = {
            { -halfSize.x, -halfSize.y, -halfSize.z },
            { -halfSize.x,  halfSize.y, -halfSize.z },
            {  halfSize.x,  halfSize.y, -halfSize.z },
            {  halfSize.x, -halfSize.y, -halfSize.z },
            { -halfSize.x, -halfSize.y,  halfSize.z },
            { -halfSize.x,  halfSize.y,  halfSize.z },
            {  halfSize.x,  halfSize.y,  halfSize.z },
            {  halfSize.x, -halfSize.y,  halfSize.z },
        };
        aabb_.min = aabb_.max = (vertices[0] + center) * transform.GetWorldMatrix();
        for (uint32_t i = 1; i < 8; ++i) {
            aabb_.Include((vertices[i] + center) * transform.GetWorldMatrix());
        }
    }
};

class CapsuleCollider : public Collider {
public:
    Vector3 start;
    Vector3 end;
    float radius;

    Vector3 FindFurthestPoint(const Vector3& direction) const override {
        Vector3 localDirection = transform.GetWorldMatrixInverse().ApplyRotation(direction);
        Vector3 point = (Dot(localDirection, start) > Dot(localDirection, end) ? start : end) * transform.GetWorldMatrix();
        float size = radius * transform.scale.Max();
        return direction.Normalized() * size + point;
    }

    void UpdateAABB() override {
        float size = radius * transform.scale.Max();
        Vector3 s = start * transform.GetWorldMatrix();
        Vector3 e = end * transform.GetWorldMatrix();

        aabb_.min = Vector3::Min(s, e);
        aabb_.max = Vector3::Max(s, e);

        aabb_.min += Vector3{ -size };
        aabb_.max += Vector3{ size };
    }
};

class MeshCollider : public Collider {
public:
    std::vector<Vector3> vertices;

    Vector3 FindFurthestPoint(const Vector3& direction) const override {
        Vector3 localDirection = transform.GetWorldMatrixInverse().ApplyRotation(direction);
        auto iter = vertices.begin();
        auto furthestPoint = iter++;
        float maxDistance = Dot(*furthestPoint, localDirection);
        for (; iter != vertices.end(); ++iter) {
            float distance = Dot(*iter, localDirection);
            if (distance > maxDistance) {
                maxDistance = distance;
                furthestPoint = iter;
            }
        }
        return *furthestPoint * transform.GetWorldMatrix();
    }

    void UpdateAABB() override {
        auto iter = vertices.begin();
        aabb_.min = aabb_.max = *iter * transform.GetWorldMatrix();
        for (; iter != vertices.end(); ++iter) {
            aabb_.Include(*iter * transform.GetWorldMatrix());
        }
    }
};