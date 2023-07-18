#pragma once

#include "Math/MathUtils.hpp"
#include "Collider.hpp"

struct GJKInfo {
    std::vector<Vector3> simplex;
};

bool GJK(const Collider& collider1, const Collider& collider2, GJKInfo* gjkInfo);
