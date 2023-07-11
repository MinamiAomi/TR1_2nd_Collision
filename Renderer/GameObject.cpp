#include "GameObject.hpp"

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