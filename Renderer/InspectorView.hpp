#pragma once
#include "ViewWindow.hpp"

#include "GameObject.hpp"

class InspectorView :
    public ViewWindow {
public:
    InspectorView() : gameObject_(nullptr) {}

    void Show() override;

    void SetGameObejct(GameObject* gameObject) { gameObject_ = gameObject; }
   
    GameObject* GetGameObject() { return gameObject_; }
    const GameObject* GetGameObject() const { return gameObject_; }

private:
    GameObject* gameObject_;
};