#pragma once
#include "Object.hpp"

#include <string>

#include "GameObject.hpp"

class Scene :
    public Object {
public:

    GameObject& AddGameObject(const std::string& name);

    std::vector<std::unique_ptr<GameObject>>& GetGameObjects() { return gameObjects_; }
    const std::vector<std::unique_ptr<GameObject>>& GetGameObjects() const { return gameObjects_; }
    GameObject& GetGameObject(size_t index) { return *gameObjects_[index]; }
    const GameObject& GetGameObject(size_t index) const { return *gameObjects_[index]; }

private:
    std::vector<std::unique_ptr<GameObject>> gameObjects_;

};