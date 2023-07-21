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
    renderer.DrawBox(local * box.transform.GetWorldMatrix(), color, DrawMode::kObject);
}
void Draw(SphereCollider sphere, Renderer& renderer, const Vector4& color) {
    float radius = sphere.radius * sphere.transform.scale.Max();
    Vector3 center = sphere.center * sphere.transform.GetWorldMatrix();
    Matrix4x4 matrix = Matrix4x4::MakeAffineTransform(Vector3{ radius }, Quaternion::identity, center);
    renderer.DrawSphere(matrix, color, DrawMode::kObject);
}

void DrawAABB(const AABB& aabb, Renderer& renderer, const Vector4& color, DrawMode drawMode = DrawMode::kWireFrame) {
    Vector3 center = aabb.Center();
    Vector3 size = aabb.Extent();
    renderer.DrawBox(Matrix4x4::MakeAffineTransform(size, Quaternion::identity, center), color, drawMode);
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
        box.transform.translate = {0.0f,float(std::distance(&boxes[0], &box)),0.0f};
    }
    boxes[0].isStatic = true;
    boxes[0].transform.translate = Vector3::down;
    boxes[0].transform.scale = { 10.0f,1.0f,10.0f };

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





                ImGui::End();


                for (auto& box : boxes) {
                    if (!box.isStatic) {
                        box.transform.translate -= {0, 0.01f, 0};
                    }
                    box.transform.UpdateWorldMatrix();
                    box.UpdateAABB();
                }

                for (auto iter1 = boxes.begin(); iter1 != boxes.end(); ++iter1) {
                    for (auto iter2 = iter1 + 1; iter2 != boxes.end(); ++iter2) {
                        if (!(iter1->isStatic && iter2->isStatic) && 
                            iter1->GetAABB().Intersects(iter2->GetAABB())) {
                            GJKInfo gjkInfo{};
                            if (GJK(*iter1, *iter2, &gjkInfo)) {
                                EPAInfo epaInfo = EPA(gjkInfo, *iter1, *iter2);
                                if (iter1->isStatic) {
                                    iter2->transform.translate += epaInfo.normal * epaInfo.depth;
                                }
                                else if (iter2->isStatic) {
                                    iter1->transform.translate -= epaInfo.normal * epaInfo.depth;
                                }
                                else {
                                    iter1->transform.translate -= epaInfo.normal * (epaInfo.depth * 0.5f);
                                    iter2->transform.translate += epaInfo.normal * (epaInfo.depth * 0.5f);

                                }
                            }

                        }
                    }
                }

                for (auto& box : boxes) {
                    box.transform.UpdateWorldMatrix();
                    box.UpdateAABB();
                }

                camera.Update();
                renderer.SetCamera(camera.position, camera.rotate);
                renderer.SetLight(light.direction, light.color, light.intensity);

                for (auto& box : boxes) {
                    Draw(box, renderer, Vector4::one);
                    DrawAABB(box.GetAABB(), renderer, Vector4::one);
                }

                renderer.EndRendering();
            }
        }
    }

    renderer.Finalize();

    return 0;
}

