#pragma once
#include "Object.hpp"

#include <string>

class GameObject;
class Transform;

class Component :
    public Object {
public:
    Component(GameObject* const gameObject);
    virtual ~Component() = 0 {}
    virtual void ShowUI() = 0 {}

    const GameObject& GetGameObject() const { return gameObject; }
    GameObject& GetGameObject() { return gameObject; }
    const Transform& GetTransform() const;
    Transform& GetTransform();

    bool CompareTag(const std::string& tag);

    GameObject& gameObject;
};