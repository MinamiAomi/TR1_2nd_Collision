#pragma once

#include "Math/MathUtils.hpp"
#include "AABB.hpp"

class Plane {
public:

    Plane();
    Plane(const Vector3& normal, float direction);
    Plane(const Vector3& point, const Vector3& normal);
    Plane(const Vector3& point0, const Vector3& point1, const Vector3& point2);

    Plane operator*(const Matrix4x4& matrix) const;

    const Vector3& GetNormal() const { return normal_; }
    float GetDirection() const { return direction_; }

private:
    Vector3 normal_;
    float direction_;
};

class Frustum {
public:
    enum CornerID {
        NearLowerLeft, NearUpperLeft, NearLowerRight, NearUpperRight,
        FarLowerLeft, FarUpperLeft, FarLowerRight, FarUpperRight,
        NumCorners
    };

    enum PlaneID {
        Near, Far, Left, Right, Top, Bottom,
        NumPlanes
    };

    Frustum();
    Frustum(const Matrix4x4& projectionMatrix);

    Frustum operator*(const Matrix4x4& matrix) const;

    AABB GetAABB() const;

    Frustum OrthoTransform(const Quaternion& rotate, const Vector3& translate);

    const Vector3& GetCorner(CornerID id) const { return corners_[id]; }
    const Plane& GetPlane(PlaneID id) const { return planes_[id]; }

private:
    Vector3 corners_[NumCorners]; // 頂点
    Plane planes_[NumPlanes];     // 平面
};