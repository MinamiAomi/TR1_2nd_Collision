#pragma once

#include "Collider.hpp"

class CollisionManager {
public:

    void AddCollider(Collider* collider);
    void Clear();
    void Solver();

private:
    std::vector<Collider*> colliders_;
};