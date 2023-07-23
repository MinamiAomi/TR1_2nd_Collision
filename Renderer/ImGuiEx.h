#pragma once

#include <format>

#include "Externals/ImGui/imgui.h"

#include "Math/MathUtils.hpp"

namespace ImGui::Ex {

    inline bool DragDegree(const char* label, float* radian, float speed = 1.0f, float min = -360.0f, float max = 360.0f, const char* format = "%.0f") {
        float deg = *radian * Math::ToDegree;
        bool result = ImGui::DragFloat(label, &deg, speed, min, max, format);
        *radian = deg * Math::ToRadian;
        return result;
    }
    inline bool DragDegree3(const char* label, float* radian, float speed = 1.0f, float min = -360.0f, float max = 360.0f, const char* format = "%.0f") {
        Vector3 deg = Vector3{ radian[0], radian[1], radian[2] } *Math::ToDegree;
        bool result = ImGui::DragFloat3(label, &deg.x, speed, min, max, format);
        radian[0] = deg.x * Math::ToRadian;
        radian[1] = deg.y * Math::ToRadian;
        radian[2] = deg.z * Math::ToRadian;
        return result;
    }
    inline bool DragDegree3(const char* label, Vector3& radian, float speed = 1.0f, float min = -360.0f, float max = 360.0f, const char* format = "%.0f") {
        return DragDegree3(label, &radian.x, speed, min, max, format);
    }
    inline bool DragVector3(const char* label, Vector3& vector, float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* format = "%.3f") {
        return ImGui::DragFloat3(label, &vector.x, speed, min, max, format);
    }

    inline void TextVector3(const char* label, const Vector3& vector) {
        auto str = std::string("%.3f, %.3f, %.3f   ") + label;
        ImGui::Text(str.c_str(), vector.x, vector.y, vector.z);
    }
}