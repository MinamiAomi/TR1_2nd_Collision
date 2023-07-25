#pragma once

#include <cstdint>
#include <vector>
#include <filesystem>

#include "Math/MathUtils.hpp"

struct ObjData {
    std::vector<Vector3> positions;
    std::vector<Vector3> normals;
    std::vector<uint16_t> indices;
};

ObjData LoadObj(const std::filesystem::path& path);