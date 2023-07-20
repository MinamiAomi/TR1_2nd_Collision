#pragma once

#include <array>

#include "Math/MathUtils.hpp"
#include "Collider.hpp"



struct GJKInfo {
    std::array<Vector3, 4> simplex;
};

struct EPAInfo {
    Vector3 normal;
    float depth;
};

bool GJK(const Collider& collider1, const Collider& collider2, GJKInfo* gjkInfo);
EPAInfo EPA(const GJKInfo& gjkInfo, const Collider& collider1, const Collider& collider2);
