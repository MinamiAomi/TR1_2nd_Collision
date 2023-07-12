#include "GameObject.hpp"

#include "Externals/ImGui/imgui.h"

GameObject::GameObject() :
    parent_(nullptr),
    transform(this),
    isActive_(true) {
}

void GameObject::SetParent(GameObject* parent) {
    if (parent_) {
        parent_->RemoveChild(this);
    }
    parent_ = parent;
    parent_->AddChild(this);
}

void GameObject::ShowUI() {
    char buf[256] = {};
    GetName().copy(buf, 256);
    if (ImGui::InputText(GetName().c_str(), buf, 256), ImGuiInputTextFlags_EnterReturnsTrue) {
        SetName(buf);
    }

    ImGui::Separator();
    transform.ShowUI();
    for (auto& component : components_) {
        ImGui::Separator();
        component->ShowUI();
    }
    for (auto& behavior : behaviors_) {
        ImGui::Separator();
        behavior->ShowUI();
    }
}

void GameObject::AddChild(GameObject* child) {
    children_.emplace_back(child);
    child->transform.parent_ = &transform;
}

void GameObject::RemoveChild(GameObject* child) {
    auto iter = std::find(children_.begin(), children_.end(), child);
    std::swap(*iter, children_.back());
    children_.pop_back();
    child->transform.parent_ = nullptr;
}

template<>
Transform* GameObject::AddComponent() {
    return &transform;
}

template<>
Transform* GameObject::GetComponent() {
    return &transform;
}

template<>
void GameObject::RemoveComponent<Transform>() {}