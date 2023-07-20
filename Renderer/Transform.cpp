#include "Transform.hpp"

#include "Externals/ImGui/imgui.h"

#include "GameObject.hpp"
#include <string>


void Transform::UpdateWorldMatrix() {
    worldMatrix_ = Matrix4x4::MakeAffineTransform(scale, rotate, translate);
    if (parent_) {
        worldMatrix_ *= parent_->GetWorldMatrix();
    }
    worldMatrixInverse_ = worldMatrix_.Inverse();
}

#ifdef _DEBUG
bool Transform::ImGuiEdit(const char* label) {
    if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3("Translate", &translate.x, 0.01f);
        ImGui::DragFloat3("Rotate", &eulerRotate_.x, 1.0f, 0.0f, 0.0f, "%.0f");
        rotate = Quaternion::MakeFromEulerAngle(eulerRotate_ * Math::ToRadian);
        ImGui::DragFloat3("Scale", &scale.x, 0.01f);
        UpdateWorldMatrix();
        ImGui::TreePop();
        return true;
    }
    return false;
}
#endif
