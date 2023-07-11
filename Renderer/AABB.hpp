#pragma once

#include "Math/MathUtils.hpp"

class AABB {
public:
    AABB() : min(Vector3::positiveInfinity), max(Vector3::negativeInfinity) {}
    AABB(const Vector3& min, const Vector3& max) : min(min), max(max) {}
    explicit AABB(const Vector3& minmax) : min(minmax), max(minmax) {}
    AABB(const Vector3& v0, const Vector3& v1, const Vector3& v2) : min(v0), max(v0) { Include(v1); Include(v2); }

    void Include(const Vector3& point) {
        min = Vector3::Min(min, point);
        max = Vector3::Max(max, point);
    }
    void Include(const AABB& other) {
        min = Vector3::Min(min, other.min);
        max = Vector3::Max(max, other.max);
    }

    Vector3 Extent() const { return max - min; }
    float Extent(size_t dim) const { return max[dim] - min[dim]; }

    Vector3 Center() const { return (max + min) * 0.5f; }
    float Center(size_t dim) const { return (max[dim] + min[dim]) * 0.5f; }

    bool Contains(const AABB& other) const {
        return
            min.x <= other.min.x &&
            other.max.x <= max.x &&
            min.y <= other.min.y &&
            other.max.y <= max.y;
    }
    bool Contains(const Vector2& point) const {
        return
            min.x <= point.x &&
            point.x <= max.x &&
            min.y <= point.y &&
            point.y <= max.y;
    }

    bool Intersects(const AABB& other) const {
        return
            min.x <= other.max.x &&
            other.min.x <= max.x &&
            min.y <= other.max.y &&
            other.min.y <= max.y;
    }
    size_t LongestAxis() const {
        Vector3 extent = Extent();
        if (extent.x >= extent.y && extent.x >= extent.z) { return 0; }
        else if (extent.y >= extent.z) { return 1; }
        return 2;
    }

    Vector3 min, max;
};