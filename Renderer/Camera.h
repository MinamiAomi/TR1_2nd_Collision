#pragma once

#include "Math/MathUtils.hpp"

class Camera {
public:
    void Update();

    void SetProjectionMatrix(float fovY, float aspectRaito, float nearZ, float farZ);

    const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
    const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }

    float GetFovY() const { return fovY_; }
    float GetAspectRaito() const { return aspectRaito_; }
    float GetNearClip() const { return nearClip; }
    float GetFarClip() const { return farClip; }

private:
    Vector3 position_ = { 0.0f,1.0f,-6.0f };
    Vector3 rotate_ = { 0,0,0 };
    Matrix4x4 viewMatrix_;

    float fovY_;
    float aspectRaito_;
    float nearClip;
    float farClip;
    Matrix4x4 projectionMatrix_;
};