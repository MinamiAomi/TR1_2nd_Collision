#include "Scene.hpp"

GameObject& Scene::AddGameObject(const std::string& name) {
    auto& go = gameObjects_.emplace_back(std::make_unique<GameObject>());
    go->SetName(name);
    return *go;
}
