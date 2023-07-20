#pragma once

#include "Math/MathUtils.hpp"

class Transform {
public:
    Transform() :
        scale(Vector3::one),
        rotate(Quaternion::identity),
        translate(Vector3::zero),
        worldMatrix_(Matrix4x4::identity),
        parent_(nullptr) {
    }

    Transform* GetParent() { return parent_; }
    const Transform* GetParent() const { return parent_; }
    const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
    const Matrix4x4& GetWorldMatrixInverse() const { return worldMatrixInverse_; }
    Vector3 GetWorldPosition() const { return worldMatrix_.GetTranslate(); }

    void SetParent(Transform* parent) { parent_ = parent; }

    void UpdateWorldMatrix();

    Vector3 scale;
    Quaternion rotate;
    Vector3 translate;

private:
    Matrix4x4 worldMatrix_;
    Matrix4x4 worldMatrixInverse_;
    Transform* parent_;

#ifdef _DEBUG
    Vector3 eulerRotate_;
public:
    bool ImGuiEdit(const char* label);
private:
#endif
};