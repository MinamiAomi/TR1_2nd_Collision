#include "Frustum.h"

Frustum::Frustum() {
}

Frustum::Frustum(const Matrix4x4& projectionMatrix) {
    const Vector3 basePoints[]{
        { -1.0f, -1.0f, 0.0f },
        { -1.0f,  1.0f, 0.0f },
        {  1.0f, -1.0f, 0.0f },
        {  1.0f,  1.0f, 0.0f },
        { -1.0f, -1.0f, 1.0f },
        { -1.0f,  1.0f, 1.0f },
        {  1.0f, -1.0f, 1.0f },
        {  1.0f,  1.0f, 1.0f },
    };

    Matrix4x4 projectionMatrixInverse = projectionMatrix.Inverse();

    for (int i = 0; i < NumCorners; ++i) {
        corners_[i] = projectionMatrixInverse.ApplyTransformWDivide(basePoints[i]);
    }

    planes_[Near] = Plane(corners_[NearLowerRight], corners_[NearUpperRight], corners_[NearUpperLeft]);
    planes_[Far] = Plane(corners_[FarLowerLeft], corners_[FarUpperLeft], corners_[FarUpperRight]);
    planes_[Left] = Plane(corners_[NearLowerLeft], corners_[NearUpperLeft], corners_[FarUpperLeft]);
    planes_[Right] = Plane(corners_[FarLowerRight], corners_[FarUpperRight], corners_[NearUpperRight]);
    planes_[Top] = Plane(corners_[FarUpperLeft], corners_[NearUpperLeft], corners_[NearUpperRight]);
    planes_[Bottom] = Plane(corners_[NearLowerLeft], corners_[FarLowerLeft], corners_[FarLowerRight]);
}

Frustum Frustum::operator*(const Matrix4x4& matrix) const {
    Frustum result;
    for (int i = 0; i < NumCorners; ++i) {
        result.corners_[i] = corners_[i] * matrix;
    }
    Matrix4x4 xFrom = matrix.Inverse().Transpose();

    for (int i = 0; i < NumPlanes; ++i) {
        result.planes_[i] = planes_[i] * matrix;
    }

    return result;
}


Plane::Plane() :
    direction_(0.0f) {
}

Plane::Plane(const Vector3& normal, float direction) :
    normal_(normal),
    direction_(direction) {
}

Plane::Plane(const Vector3& point, const Vector3& normal) {
    normal_ = normal.Normalized();
    direction_ = Dot(point, normal_);
}

Plane::Plane(const Vector3& point0, const Vector3& point1, const Vector3& point2) {
    Plane(point0, Cross(point1 - point0, point2 - point1));
}

Plane Plane::operator*(const Matrix4x4& matrix) const {
    Vector4 v(normal_, direction_);
    v = v * matrix;
    return Plane(v.GetXYZ(), v.w);
}

AABB Frustum::GetAABB() const {
    AABB aabb(corners_[0]);
    for (int i = 1; i < NumCorners; ++i) {
        aabb.Include(corners_[i]);
    }
    return aabb;
}
