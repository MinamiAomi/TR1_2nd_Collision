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

private:
    std::vector<std::unique_ptr<GameObject>> gameObjects_;

};