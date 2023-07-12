#include "Transform.hpp"

#include "Externals/ImGui/imgui.h"

#include "GameObject.hpp"

void Transform::ShowUI() {
    if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding)) {
        ImGui::DragFloat3("Translate", &translate.x, 0.1f);
        ImGui::DragFloat3("Rotate", &translate.x);
        ImGui::DragFloat3("Scale", &scale.x, 0.1f);

        ImGui::TreePop();
    }
}

void Transform::UpdateWorldMatrix() {
    worldMatrix_ = Matrix4x4::MakeAffineTransform(scale, rotate, translate);
    if (parent_) {
        worldMatrix_ *= parent_->GetWorldMatrix();
    }
}
