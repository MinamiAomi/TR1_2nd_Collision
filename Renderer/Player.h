#pragma once

#include "Collider.hpp"
#include "Camera.h"

class Player : public CapsuleCollider {
public:

    void Initialize();
    void CameraUpdate();
    void Update();
    void UpdateMatrix();

    const Matrix4x4& GetViewMatrix() const { return camera.GetWorldMatrixInverse(); }

    void ImGuiEdit();

private:
    void PlayerMove();

    void OnCollision(const CollisionInfo& info);

    float jumpVelocity = 0.0f;
    bool canJump = true;

    Transform viewPoint;
    Transform camera;
    float cameraDistance = 7.0f;
    Vector2 cameraSphericalAngle;
};