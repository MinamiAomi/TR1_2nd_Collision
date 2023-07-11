#pragma once

#include <string>

class GameObject;
class Transform;

class Component {
public:
    Component(GameObject* const gameObject);
    virtual ~Component() = 0 {}

    const GameObject& GetGameObject() const { return gameObject; }
    GameObject& GetGameObject() { return gameObject; }
    const Transform& GetTransform() const;
    Transform& GetTransform();

    bool CompareTag(const std::string& tag);

    GameObject& gameObject;
};