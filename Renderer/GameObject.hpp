#pragma once

#include <memory>
#include <type_traits>
#include <string>
#include <vector>

#include "Component.hpp"
#include "Behavior.hpp"
#include "Transform.hpp"

class GameObject {
private:
    template<class T>
    using ComponentType = std::conditional<std::is_base_of<Behavior, T>::value, Behavior, Component>::type;

public:
    GameObject();

    template<class T>
    T* AddComponent() {
        static_assert(std::is_base_of<Component, T>::value, "Not inherit Component");
        T* component = new T(this);
        SelecteStorage<T>().emplace_back(component);
        return component;
    }
    template<class T>
    T* GetComponent() {
        T* component;
        for (auto& c : SelecteStorage<T>()) {
            component = dynamic_cast<T*>(c.get());
            if (component) { return component; }
        }
        return nullptr;
    }
    template<class T>
    void RemoveComponent() {
        auto& v = SelecteStorage<T>();
        auto iter = std::find_if(v.begin(), v.end(),
            [](const auto& component) { return dynamic_cast<T*>(component.get()); });
        std::swap(*iter, v.back());
        v.pop_back();
    }

    void SetTag(const std::string& tag) { tag_ = tag; }
    void SetIsActive(bool isActive) { isActive_ = isActive; }
    void SetParent(GameObject* parent);

    bool IsActive() const { return isActive_; }
    const std::string& GetTag() const { return tag_; }
    Transform* GetTransform() { return &transform; }
    const Transform* GetTransform() const { return &transform; }

    Transform transform;

private:
    void AddChild(GameObject* child);
    void RemoveChild(GameObject* child);

    template<class T>
    constexpr std::vector<std::unique_ptr<ComponentType<T>>>& SelecteStorage() {
        if constexpr (std::is_same<ComponentType<T>, Behavior>::value) {
            return behaviors_;
        }
        else {
            return components_;
        }
    }

    std::string tag_;
    GameObject* parent_;
    std::vector<GameObject*> children_;
    std::vector<std::unique_ptr<Component>> components_;
    std::vector<std::unique_ptr<Behavior>> behaviors_;
    bool isActive_;
};
