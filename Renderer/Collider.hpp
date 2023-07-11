#pragma once

#include "Math/MathUtils.hpp"
#include "AABB.hpp"

#include <functional>

class Collider {
public:
    using CollBack = std::function<void(void)>;

    virtual ~Collider() {}
    virtual Vector3 FindFurthestPoint(const Vector3& direction) const = 0;

    void SetIsActive(bool isActive) { isActive_ = isActive; }
    void SetIsTrigger(bool isTrigger) { isTrigger_ = isTrigger; }
    void SetEnterCollBack(const CollBack& collBack) { enterCollBack_ = collBack; }
    void SetStayCollBack(const CollBack& collBack) { stayCollBack_ = collBack; }
    void SetExitCollBack(const CollBack& collBack) { exitCollBack_ = collBack; }
 
    bool IsActive() const { return isActive_; }
    bool IsTrigger() const { return isTrigger_; }
    const CollBack& GetEnterCollBack() const { return enterCollBack_; }
    const CollBack& GetStayCollBack() const { return stayCollBack_; }
    const CollBack& GetExitCollBack() const { return exitCollBack_; }
    const AABB& GetAABB() const { return aabb_; }

protected:
    AABB aabb_{};

private:
    CollBack enterCollBack_;
    CollBack stayCollBack_;
    CollBack exitCollBack_;
    bool isActive_;
    bool isTrigger_;
};

