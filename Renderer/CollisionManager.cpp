#include "CollisionManager.h"

#include "Collision.h"

void CollisionManager::AddCollider(Collider* collider) {
    colliders_.emplace_back(collider);
}

void CollisionManager::Clear() {
    colliders_.clear();
}

void CollisionManager::Solver() {
    for (auto collider : colliders_) {
        if (!collider->isStatic) {
            collider->UpdateAABB();
        }
    }


    for (auto iter1 = colliders_.begin(); iter1 != colliders_.end(); ++iter1) {
        for (auto iter2 = (iter1 + 1); iter2 != colliders_.end(); ++iter2) {

            auto collider1 = *iter1, collider2 = *iter2;

            // 静的同士は判定を取らない
            if (collider1->isStatic && collider2->isStatic) { continue; }
            // AABBが当たってない
            if (!collider1->GetAABB().Intersects(collider2->GetAABB())) { continue; }
            GJKInfo gjkInfo{};
            if (GJK(*collider1, *collider2, &gjkInfo)) {
                EPAInfo epaInfo = EPA(gjkInfo, *collider1, *collider2);

                CollisionInfo collisionInfo1{}, collisionInfo2{};
                collisionInfo1.collider = collider2;
                collisionInfo2.collider = collider1;
                collisionInfo1.normal = -epaInfo.normal;
                collisionInfo2.normal = epaInfo.normal;

                // 1が静的
                if (collider1->isStatic && !collider2->isStatic) {
                    collider2->transform.translate += epaInfo.normal * epaInfo.depth;

                    collisionInfo1.depth = 0.0f;
                    collisionInfo2.depth = epaInfo.depth;
                }
                // 2が静的
                else if (!collider1->isStatic && collider2->isStatic) {
                    collider1->transform.translate -= epaInfo.normal * epaInfo.depth;

                    collisionInfo1.depth = epaInfo.depth;
                    collisionInfo2.depth = 0.0f;
                }
                // どちらも動的
                else {
                    float depth = epaInfo.depth * 0.5f;
                    collider1->transform.translate -= epaInfo.normal * depth;
                    collider2->transform.translate += epaInfo.normal * depth;

                    collisionInfo1.depth = depth;
                    collisionInfo2.depth = depth;
                }

                collider1->CallBack(collisionInfo1);
                collider2->CallBack(collisionInfo2);
            }
        }
    }

    for (auto collider : colliders_) {
        if (!collider->isStatic) {
            collider->transform.UpdateWorldMatrix();
            collider->UpdateAABB();
        }
    }

}
