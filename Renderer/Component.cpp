#include "Component.hpp"

#include "GameObject.hpp"

Component::Component(GameObject* const gameObject) :
    gameObject(*gameObject) {
}

const Transform& Component::GetTransform() const {
    return gameObject.transform;
}

Transform& Component::GetTransform() {
    return gameObject.transform;
}

bool Component::CompareTag(const std::string& tag) {
    return tag == gameObject.GetTag();
}
