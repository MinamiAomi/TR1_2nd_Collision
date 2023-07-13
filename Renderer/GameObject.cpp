#include "GameObject.hpp"

#include "Externals/ImGui/imgui.h"

GameObject::GameObject() :
    parent_(nullptr),
    transform(this),
    isActive_(true) {
}

void GameObject::SetParent(GameObject* parent) {
    if (parent) {
        GameObject* p = parent;
        while (p->parent_) {
            p = p->parent_;
            if (p == this) {
                parent->parent_->RemoveChild(parent);
                parent->parent_ = parent_;
                parent_ = parent;
                break;
            }
        }
    }

    if (parent_) {
        parent_->RemoveChild(this);
    }
    parent_ = parent;
    if (parent_) {
        parent_->AddChild(this);
    }
}

void GameObject::ShowUI() {
    ImGui::Checkbox("Active", &isActive_);
    char buf[256] = {};
    GetName().copy(buf, 256);
    ImGui::SameLine();
    if (ImGui::InputText("Name", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
        SetName(buf);
    }

    ImGui::Separator();
    transform.ShowUI();
    ImGui::Separator();
    for (auto& component : components_) {
        component->ShowUI();
        ImGui::Separator();
    }
    for (auto& behavior : behaviors_) {
        behavior->ShowUI();
        ImGui::Separator();
    }
}

void GameObject::AddChild(GameObject* child) {
    children_.emplace_back(child);
    child->transform.parent_ = &transform;
}

void GameObject::RemoveChild(GameObject* child) {
    if (child) {
        auto iter = std::find(children_.begin(), children_.end(), child);
        if (iter != children_.end()) {
            std::swap(*iter, children_.back());
            children_.pop_back();
            child->transform.parent_ = nullptr;
        }
    }
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