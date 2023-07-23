#include "Player.h"

#include "Input.hpp"
#include "ImGuiEx.h"

void Player::Initialize() {
    transform.translate = { 0.0f,1.0f, 0.0f };

    start = { 0.0f,0.5f,0.0f };
    end = { 0.0f,-0.5f,0.0f };
    radius = 0.5f;

    viewPoint.SetParent(&transform);
    viewPoint.translate = { 0.0f,1.5f,0.0f };
    camera.SetParent(&viewPoint);
    camera.translate = -Vector3::unitZ * cameraDistance;
    camera.rotate = Quaternion::MakeLookRotation(-camera.translate);

    SetCallBack([this](const CollisionInfo& info) { OnCollision(info); });
}

void Player::Update() {
    PlayerMove();

    UpdateMatrix();
}

void Player::UpdateMatrix() {

    transform.UpdateWorldMatrix();
    viewPoint.UpdateWorldMatrix();
    camera.UpdateWorldMatrix();
}

void Player::ImGuiEdit() {
    ImGui::Begin("Player");
    transform.ImGuiEdit("Transform");

    ImGui::End();
}

void Player::PlayerMove() {
    constexpr float moveSpeed = 0.1f;
    constexpr float jumpPower = 5.5f;
    constexpr float deltaTime = 1.0f / 60.0f;

    Vector3 forward = camera.rotate * Vector3::unitZ;
    forward.y = 0.0f;
    forward = forward.Normalized();
    Vector3 right = Cross(Vector3::up, forward);

    Vector3 move;

    if (Input::IsKeyPressed(Keycode::W)) {
        move += forward * moveSpeed;
    }
    if (Input::IsKeyPressed(Keycode::S)) {
        move -= forward * moveSpeed;
    }
    if (Input::IsKeyPressed(Keycode::D)) {
        move += right * moveSpeed;
    }
    if (Input::IsKeyPressed(Keycode::A)) {
        move -= right * moveSpeed;
    }

    if (Input::IsKeyTriggered(Keycode::Space) && canJump) {
        jumpVelocity = jumpPower;
        canJump = false;
    }

    jumpVelocity -= 9.8f * deltaTime;

    transform.translate += move + Vector3::up * jumpVelocity * deltaTime;
}

void Player::OnCollision(const CollisionInfo& info) {
    const float normalAngleDiffLimit = std::cos(45.0f * Math::ToRadian);
    float dot = Dot(info.normal, Vector3::up);
    if (std::abs(dot) > normalAngleDiffLimit) {
        jumpVelocity = 0.0f;
        if (dot > 0.0f) {
            canJump = true;
        }
    }
}

void Player::CameraUpdate() {
    constexpr float horizontalAimingSpeed = 0.4f * Math::ToRadian;
    constexpr float verticalAimingSpeed = 0.2f * Math::ToRadian;
    constexpr float verticalAngleLimit = 80.0f * Math::ToRadian;
    if (!Input::IsMousePressed(MouseButton::Right)) {

        Vector2 mouseMove = Input::GetMouseMove();
        cameraSphericalAngle.x += mouseMove.y * verticalAimingSpeed;
        cameraSphericalAngle.y += mouseMove.x * horizontalAimingSpeed;

        // 角度制限
        cameraSphericalAngle.y = std::fmod(cameraSphericalAngle.y, Math::TwoPi);
        cameraSphericalAngle.x = std::clamp(cameraSphericalAngle.x, -verticalAngleLimit, verticalAngleLimit);

        camera.translate = Quaternion::MakeFromEulerAngle((Vector3)cameraSphericalAngle) * -Vector3::unitZ * cameraDistance;
        camera.rotate = Quaternion::MakeLookRotation(-camera.translate);

        UpdateMatrix();
    }
}
