#pragma once

#include "Component.hpp"

class Behavior :
    public Component {
public:
    Behavior(GameObject* const gameObject) :
        Component(gameObject), isEnabled_(true) {}
    virtual ~Behavior() {}

    virtual void Initalize() = 0;
    virtual void Update() = 0;

    void SetIsEnabled(bool isEnabled) { isEnabled_ = isEnabled; }
    bool IsEnabled() const { return isEnabled_; }

private:
    bool isEnabled_;
};