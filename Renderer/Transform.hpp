#pragma once
#include "Component.hpp"

#include "Math/MathUtils.hpp"

class Transform : 
    public Component{
public:
    Transform(GameObject* const gameObject) :
        Component(gameObject),
        scale(Vector3::one),
        rotate(Quaternion::identity),
        translate(Vector3::zero),
        worldMatrix_(Matrix4x4::identity),
        parent_(nullptr) {
    }

    Transform* GetParent() { return parent_; }
    const Transform* GetParent() const { return parent_; }
    const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
    Vector3 GetWorldPosition() const { return worldMatrix_.GetTranslate(); }

    void UpdateWorldMatrix();

    Vector3 scale;
    Quaternion rotate;
    Vector3 translate;

private:
    Matrix4x4 worldMatrix_;
    Transform* parent_;

    friend class GameObject;
};