#include <Windows.h>

#include <stack>

#include "Externals/ImGui/imgui.h"

#include "Math/MathUtils.hpp"
#include "Renderer.hpp"
#include "ShaderUtils.hpp"
#include "Utils.hpp"

#include "Input.hpp"

// 定数
constexpr uint32_t kSwapChainBufferCount = 2;
// クライアント領域サイズ
const uint32_t kClientWidth = 1280;
const uint32_t kClientHeight = 720;

struct CollisionObject {
    Vector3 translate;
    Quaternion rotate;
    Vector3 scale{ 1,1,1 };
    Matrix4x4 matrix;

    void UpdateMatrix();
};

struct Camera {
    Vector3 position = { 0.0f,1.0f,-6.0f };
    Vector3 rotate = { 0,0,0 };
    void Update() {
        if (Input::IsMousePressed(MouseButton::Right)) {
            constexpr float rotSpeed = 1.0f * Math::ToRadian;
            rotate.x += rotSpeed * Input::GetMouseMove().y * 0.1f;
            rotate.y += rotSpeed * Input::GetMouseMove().x * 0.1f;
        }
        else if (Input::IsMousePressed(MouseButton::Mid)) {
            Matrix4x4 rotMat = Matrix4x4::MakeRotationXYZ(rotate);
            Vector3 cameraX = rotMat.GetXAxis() * Input::GetMouseMove().x * -0.01f;
            Vector3 cameraY = rotMat.GetYAxis() * Input::GetMouseMove().y * 0.01f;
            position += cameraX + cameraY;
        }
        else if (Input::GetWheel()) {
            Matrix4x4 rotMat = Matrix4x4::MakeRotationXYZ(rotate);
            Vector3 cameraZ = rotMat.GetZAxis() * (Input::GetWheel() / 120 * 0.5f);
            position += cameraZ;
        }

    }
};
struct DirectionalLight {
    Vector3 direction{1,0,0};
    Vector4 color{1,1,1,1};
    float intensity{1};
};

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {


    Renderer renderer;
    renderer.Initailize(L"RendererTest", 1280, 720);
    Input::Initialize();


    CollisionObject obj1;
    obj1.translate = { 1,0,0 };

    CollisionObject obj2;
    obj2.translate = { -1,0,0 };


    Camera camera;
    DirectionalLight light;

    bool mouse = false;
    bool key = false;

    {
        MSG msg{};
        // ウィンドウの×ボタンがが押されるまでループ
        while (msg.message != WM_QUIT) {
            // Windowにメッセージが来てたら最優先で処理させる
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else {
                Input::Update();
                renderer.StartRendering();

                ImGui::Begin("Window");
                ImGui::DragFloat3("Direction", &light.direction.x, 0.01f);
                light.direction = light.direction.Normalized();
                ImGui::ColorEdit4("color", &light.color.x);
                ImGui::DragFloat("intensity", &light.intensity, 0.01f);



                ImGui::End();

                
                
                
                obj1.UpdateMatrix();
                obj2.UpdateMatrix();

                
                
                
                camera.Update();
                renderer.SetCamera(camera.position, camera.rotate);
                renderer.SetLight(light.direction, light.color, light.intensity);



                renderer.DrawBox(obj1.matrix, mouse ? Vector4{1, 0, 0, 1} : Vector4::one, DrawMode::kObject);
                renderer.DrawSphere(obj2.matrix, key ? Vector4{ 1, 0, 0, 1 } : Vector4::one, DrawMode::kObject);

                renderer.EndRendering();
            }
        }
    }

    renderer.Finalize();

    return 0;
}

void CollisionObject::UpdateMatrix() {
    matrix = Matrix4x4::MakeAffineTransform(scale, rotate, translate);
}
