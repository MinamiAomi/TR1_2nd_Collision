#pragma once

#include <vector>

#include "Math/MathUtils.hpp"
#include "AABB.hpp"
#include "Transform.hpp"

class Collider {
public:

    virtual ~Collider() {}
    virtual Vector3 FindFurthestPoint(const Vector3& direction) const = 0;

    const AABB& GetAABB() const { return aabb_; }

    Transform transform;

protected:
    AABB aabb_{};
};

class SphereCollider : public Collider {
public:
    Vector3 center;
    float radius{ 0.5f };

    Vector3 FindFurthestPoint(const Vector3& direction) const override {
        return direction.Normalized() * (radius * transform.scale.Max()) + center * transform.GetWorldMatrix();
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
};