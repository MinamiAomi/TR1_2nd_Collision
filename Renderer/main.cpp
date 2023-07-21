#include <Windows.h>

#include <stack>

#include "Externals/ImGui/imgui.h"

#include "Math/MathUtils.hpp"
#include "Renderer.hpp"
#include "ShaderUtils.hpp"
#include "Utils.hpp"

#include "Collider.hpp"
#include "Input.hpp"
#include "Transform.hpp"
#include "Collision.h"

// 定数
constexpr uint32_t kSwapChainBufferCount = 2;
// クライアント領域サイズ
const uint32_t kClientWidth = 1280;
const uint32_t kClientHeight = 720;


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
    Vector3 direction{ 1,0,0 };
    Vector4 color{ 1,1,1,1 };
    float intensity{ 1 };
};


void Draw(BoxCollider box, Renderer& renderer, const Vector4& color) {
    Matrix4x4 local = Matrix4x4::MakeAffineTransform(box.size, Quaternion::identity, box.center);
    renderer.DrawBox(local * box.transform.GetWorldMatrix(), color, DrawMode::kWireFrame);
}
void Draw(SphereCollider sphere, Renderer& renderer, const Vector4& color) {
    float radius = sphere.radius * sphere.transform.scale.Max();
    Vector3 center = sphere.center * sphere.transform.GetWorldMatrix();
    Matrix4x4 matrix = Matrix4x4::MakeAffineTransform(Vector3{ radius }, Quaternion::identity, center);
    renderer.DrawSphere(matrix, color, DrawMode::kWireFrame);
}

void DragDegree(const char* label, float& radian, float speed = 1.0f, float min = -360.0f, float max = 360.0f, const char* format = "%.0f") {
    float deg = radian * Math::ToDegree;
    ImGui::DragFloat(label, &deg, speed, min, max, format);
    radian = deg * Math::ToRadian;
}
void DragDegree3(const char* label, Vector3& radian, float speed = 1.0f, float min = -360.0f, float max = 360.0f, const char* format = "%.0f") {
    Vector3 deg = radian * Math::ToDegree;
    ImGui::DragFloat3(label, &deg.x, speed, min, max, format);
    radian = deg * Math::ToRadian;
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

    std::srand((unsigned int)std::time(nullptr));

    Renderer renderer;
    renderer.Initailize(L"RendererTest", 1280, 720);
    Input::Initialize();




    Camera camera;
    DirectionalLight light;
    Vector3 directionRotate;

    std::vector<BoxCollider> boxes(10);
    for (auto& box : boxes) {
       
    }
    

    BoxCollider sphere;
    sphere.transform.translate = { -1,0,0 };
    BoxCollider box;
    box.transform.translate = { 1 ,0,0 };

    Vector4 color{1.0f};

    Transform simplex[4];
    for (auto& t : simplex) {
        t.scale = Vector3{ 0.05f };
    }

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

                ImGui::Begin("Camera");
                Vector3 cameraDir = Matrix4x4::MakeRotationXYZ(camera.rotate).GetZAxis();
                ImGui::InputFloat3("Direction", &cameraDir.x);
                ImGui::End();

                ImGui::Begin("Window");

                DragDegree3("Direction rotate", directionRotate);
                light.direction = Quaternion::MakeFromEulerAngle(directionRotate) * Vector3::unitZ;


                sphere.transform.ImGuiEdit("Sphere transform");

               /* ImGui::DragFloat3("Sphere center", &sphere.center.x, 0.01f);
                ImGui::DragFloat("Sphere radius", &sphere.radius, 0.01f);*/
                box.transform.ImGuiEdit("Box transform");
                ImGui::DragFloat3("Box center", &box.center.x, 0.01f);
                ImGui::DragFloat3("Box size", &box.size.x, 0.01f);


                ImGui::End();


                sphere.transform.UpdateWorldMatrix();
                box.transform.UpdateWorldMatrix();

                color = Vector4::one;
                GJKInfo gjkInfo{};
                if (GJK(sphere, box, &gjkInfo)) {
                    EPAInfo epaInfo = EPA(gjkInfo, sphere, box);
                    color = { 1,0,0,1 };
                    
                    sphere.transform.translate -= epaInfo.normal * epaInfo.depth * 0.5f;
                    box.transform.translate += epaInfo.normal * epaInfo.depth * 0.5f;

                    sphere.transform.UpdateWorldMatrix();
                    box.transform.UpdateWorldMatrix();

                    for (size_t i = 0; i < 4; ++i) {
                        simplex[i].translate = gjkInfo.simplex[i];
                        simplex[i].UpdateWorldMatrix();
                    }
               }

                camera.Update();
                renderer.SetCamera(camera.position, camera.rotate);
                renderer.SetLight(light.direction, light.color, light.intensity);

                renderer.DrawSphere(Matrix4x4::MakeAffineTransform(Vector3(0.05f), Quaternion::identity, {}), { 0,0,0,1 }, DrawMode::kObject);
                Draw(sphere, renderer, color);
                Draw(box, renderer, color);

                if (color != Vector4::one) {
                    for (auto& t : simplex) {
                        renderer.DrawSphere(t.GetWorldMatrix(), {0,1,1,1}, DrawMode::kObject);
                    }
                }


                renderer.EndRendering();
            }
        }
    }

    renderer.Finalize();

    return 0;
}

