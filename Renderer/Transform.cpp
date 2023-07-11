#include "Transform.hpp"

#include "GameObject.hpp"

void Transform::UpdateWorldMatrix() {
    worldMatrix_ = Matrix4x4::MakeAffineTransform(scale, rotate, translate);
    if (parent_) {
        worldMatrix_ *= parent_->GetWorldMatrix();
    }
}
