#include <Windows.h>

#include <stack>

#include "Externals/ImGui/imgui.h"

#include "Math/MathUtils.hpp"
#include "Renderer.hpp"

#include "Collider.hpp"
#include "Input.hpp"
#include "Transform.hpp"
#include "Collision.h"
#include "Camera.h"
#include "ImGuiEx.h"
#include "Player.h"
#include "CollisionManager.h"
#include "ObjLoad.h"

#include "Frustum.h"

// クライアント領域サイズ
const uint32_t kWindowWidth = 1280;
const uint32_t kWindowHeight = 720;


struct DirectionalLight {
    Vector3 direction{ 1,0,0 };
    Vector4 color{ 1,1,1,1 };
    float intensity{ 1 };
};


void DrawBox(const BoxCollider& box, Renderer& renderer, const Vector4& color) {
    Matrix4x4 local = Matrix4x4::MakeAffineTransform(box.size, Quaternion::identity, box.center);
    renderer.DrawBox(local * box.transform.GetWorldMatrix(), color, Renderer::DrawMode::Object);
}
void DrawSphere(const SphereCollider& sphere, Renderer& renderer, const Vector4& color) {
    float radius = sphere.radius * sphere.transform.scale.Max();
    Vector3 center = sphere.center * sphere.transform.GetWorldMatrix();
    Matrix4x4 matrix = Matrix4x4::MakeAffineTransform(Vector3{ radius }, Quaternion::identity, center);
    renderer.DrawSphere(matrix, color, Renderer::DrawMode::Object);
}
void DrawCapsule(const CapsuleCollider& capsule, Renderer& renderer, const Vector4& color) {

    float radius = capsule.radius * capsule.transform.scale.Max();
    Vector3 start = capsule.start * capsule.transform.GetWorldMatrix();
    Vector3 end = capsule.end * capsule.transform.GetWorldMatrix();
    Vector3 se = end - start;
    float length = se.Length();
    Vector3 center = (start + end) * 0.5f;
    Quaternion rotate;
    if (std::abs(Dot(se.Normalized(), Vector3::unitX)) <= 0.999f) {
        Vector3 y = se.Normalized();
        Vector3 z = Vector3::Cross(Vector3::unitX, y).Normalized();
        Vector3 x = Vector3::Cross(y, z);
        rotate = Quaternion::MakeFromOrthonormal(x, y, z);
    }
    else {
        rotate = Quaternion::MakeFromAngleAxis(Math::HalfPi, Vector3::unitZ);
    }

    Matrix4x4 startSphere = Matrix4x4::MakeAffineTransform(Vector3{ radius }, rotate, start);
    Matrix4x4 endSphere = Matrix4x4::MakeAffineTransform(Vector3{ radius }, rotate, end);
    Matrix4x4 cylinder = Matrix4x4::MakeAffineTransform({ radius, length, radius }, rotate, center);
    renderer.DrawSphere(startSphere, color, Renderer::DrawMode::Object);
    renderer.DrawSphere(endSphere, color, Renderer::DrawMode::Object);
    renderer.DrawCylinder(cylinder, color, Renderer::DrawMode::Object);
}


void DrawAABB(const AABB& aabb, Renderer& renderer, const Vector4& color, Renderer::DrawMode drawMode = Renderer::DrawMode::WireFrame) {
    Vector3 center = aabb.Center();
    Vector3 size = aabb.Extent();
    renderer.DrawBox(Matrix4x4::MakeAffineTransform(size, Quaternion::identity, center), color, drawMode);
}

class World {
public:
    void Initialize(CollisionManager& collisionManager);
    void Draw(Renderer& renderer, bool showAABB);
private:
    std::vector<std::unique_ptr<BoxCollider>> boxObjects_;
    std::vector<std::unique_ptr<SphereCollider>> sphereObjects_;
    std::vector<std::unique_ptr<CapsuleCollider>> capsuleObjects_;
};

void DrawFrustum(const Frustum& frustum, Renderer& renderer, const Vector4& color) {
    renderer.DrawLine(frustum.GetCorner(Frustum::NearLowerLeft), frustum.GetCorner(Frustum::NearUpperLeft), color);
    renderer.DrawLine(frustum.GetCorner(Frustum::NearUpperLeft), frustum.GetCorner(Frustum::NearUpperRight), color);
    renderer.DrawLine(frustum.GetCorner(Frustum::NearUpperRight), frustum.GetCorner(Frustum::NearLowerRight), color);
    renderer.DrawLine(frustum.GetCorner(Frustum::NearLowerRight), frustum.GetCorner(Frustum::NearLowerLeft), color);

    renderer.DrawLine(frustum.GetCorner(Frustum::NearLowerLeft), frustum.GetCorner(Frustum::FarLowerLeft), color);
    renderer.DrawLine(frustum.GetCorner(Frustum::NearUpperLeft), frustum.GetCorner(Frustum::FarUpperLeft), color);
    renderer.DrawLine(frustum.GetCorner(Frustum::NearUpperRight), frustum.GetCorner(Frustum::FarUpperRight), color);
    renderer.DrawLine(frustum.GetCorner(Frustum::NearLowerRight), frustum.GetCorner(Frustum::FarLowerRight), color);

    renderer.DrawLine(frustum.GetCorner(Frustum::FarLowerLeft), frustum.GetCorner(Frustum::FarUpperLeft), color);
    renderer.DrawLine(frustum.GetCorner(Frustum::FarUpperLeft), frustum.GetCorner(Frustum::FarUpperRight), color);
    renderer.DrawLine(frustum.GetCorner(Frustum::FarUpperRight), frustum.GetCorner(Frustum::FarLowerRight), color);
    renderer.DrawLine(frustum.GetCorner(Frustum::FarLowerRight), frustum.GetCorner(Frustum::FarLowerLeft), color);
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

    CG::DX12::D3DResourceLeakChecker leakChecker;
    {
        std::srand((unsigned int)std::time(nullptr));

        Renderer renderer;
        renderer.Initialize(kWindowWidth, kWindowHeight);

        Input::Initialize();

        Camera camera;
        camera.SetProjectionMatrix(45.0f * Math::ToRadian, float(kWindowWidth) / kWindowHeight, 0.1f, 100.0f);

        DirectionalLight light;
        Vector3 lightRotate;
        lightRotate.x = 90.0f * Math::ToRadian;
        //lightRotate.y = 45.0f * Math::ToRadian;

        Player player;
        player.Initialize();

        CollisionManager collisionManager;
        collisionManager.AddCollider(&player);

        World world;
        world.Initialize(collisionManager);

        bool useDebugCamera = false;
        bool showAABB = false;

        std::vector<Renderer::MeshHandle> meshes;
        std::vector<std::pair<std::unique_ptr<MeshCollider>, Renderer::MeshHandle>> meshColliders;
        {
            auto suzanne = LoadObj("suzanne.obj");
            std::vector<Renderer::Vertex> vertices(suzanne.positions.size());
            for (size_t i = 0; i < suzanne.positions.size(); ++i) {
                vertices[i].position = suzanne.positions[i];
                vertices[i].normal = suzanne.normals[i];
            }
            meshes.emplace_back(renderer.RegisterMesh(vertices, suzanne.indices));

            meshColliders.emplace_back(std::make_unique<MeshCollider>(), meshes.back());
            meshColliders.back().first->vertices = suzanne.positions;
            meshColliders.back().first->transform.translate = { 1.0f,1.0f,0.0f };
            collisionManager.AddCollider(meshColliders.back().first.get());
        }
        {
            auto boxex = LoadObj("boxex.obj");
            std::vector<Renderer::Vertex> vertices(boxex.positions.size());
            for (size_t i = 0; i < boxex.positions.size(); ++i) {
                vertices[i].position = boxex.positions[i];
                vertices[i].normal = boxex.normals[i];
            }
            meshes.emplace_back(renderer.RegisterMesh(vertices, boxex.indices));

            meshColliders.emplace_back(std::make_unique<MeshCollider>(), meshes.back());
            meshColliders.back().first->vertices = boxex.positions;
            meshColliders.back().first->transform.translate = { 1.0f,1.0f,10.0f };
            collisionManager.AddCollider(meshColliders.back().first.get());
        }
        {
            auto tetora = LoadObj("tetora.obj");
            std::vector<Renderer::Vertex> vertices(tetora.positions.size());
            for (size_t i = 0; i < tetora.positions.size(); ++i) {
                vertices[i].position = tetora.positions[i];
                vertices[i].normal = tetora.normals[i];
            }
            meshes.emplace_back(renderer.RegisterMesh(vertices, tetora.indices));

            meshColliders.emplace_back(std::make_unique<MeshCollider>(), meshes.back());
            meshColliders.back().first->vertices = tetora.positions;
            meshColliders.back().first->isStatic = true;
            meshColliders.back().first->transform.translate = { -5.0f,0.0f,10.0f };
            meshColliders.back().first->transform.UpdateWorldMatrix();
            meshColliders.back().first->UpdateAABB();
            collisionManager.AddCollider(meshColliders.back().first.get());
        }
        {
            auto tera = LoadObj("tera.obj");
            std::vector<Renderer::Vertex> vertices(tera.positions.size());
            for (size_t i = 0; i < tera.positions.size(); ++i) {
                vertices[i].position = tera.positions[i];
                vertices[i].normal = tera.normals[i];
            }
            meshes.emplace_back(renderer.RegisterMesh(vertices, tera.indices));

            meshColliders.emplace_back(std::make_unique<MeshCollider>(), meshes.back());
            meshColliders.back().first->vertices = tera.positions;
            meshColliders.back().first->isStatic = true;
            meshColliders.back().first->transform.translate = { 0.0f,0.0f,30.0f };
            meshColliders.back().first->transform.scale = { 1.0f,1.0f,1.0f };
            meshColliders.back().first->color = { 0.0f,0.2f,0.0f,1.0f };
            meshColliders.back().first->transform.UpdateWorldMatrix();
            meshColliders.back().first->UpdateAABB();
            collisionManager.AddCollider(meshColliders.back().first.get());

        }
        light.direction = Quaternion::MakeFromEulerAngle(lightRotate) * Vector3::unitZ;
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
                    // 更新
                    {
                        Input::Update();
                        if (useDebugCamera) {
                            camera.Update();
                        }
                        else {
                            player.CameraUpdate();
                        }

                        player.Update();
                        for (auto& mesh : meshColliders) {
                            mesh.first->transform.UpdateWorldMatrix();
                        }

                        collisionManager.Solver();

                        player.UpdateMatrix();
                    }
                    // 描画
                    {   
                        
                        renderer.StartRendering();
                       /* {
                            Matrix4x4 lightViewMatrix = Matrix4x4::MakeAffineInverse(Matrix4x4::MakeLookRotation(light.direction, Vector3::unitX), -light.direction * 50.0f);
                            Matrix4x4 lightProjMatrix = Matrix4x4::MakeOrthographicProjection(2048.0f, 2048.0f, 1.0f, 100.0f);
                        renderer.SetViewProjectionMatrix(lightViewMatrix, lightProjMatrix);
                        }*/
                        renderer.SetViewProjectionMatrix(useDebugCamera ? camera.GetViewMatrix() : player.GetViewMatrix(), camera.GetProjectionMatrix());
                        renderer.SetDirectionalLight(light.direction, light.color, light.intensity);


                        world.Draw(renderer, showAABB);
                        if (showAABB) {
                            DrawAABB(player.GetAABB(), renderer, { 0,0,0,1 });
                        }
                        DrawCapsule(player, renderer, Vector4::one);



                        for (auto& mesh : meshColliders) {
                            if (showAABB) {
                                DrawAABB(mesh.first->GetAABB(), renderer, { 0,0,0,1 });
                            }
                            renderer.DrawMesh(mesh.second, mesh.first->transform.GetWorldMatrix(), mesh.first->color, Renderer::DrawMode::Object);
                        }

                        renderer.DrawLine(Vector3::right * 1000, Vector3::left * 1000, { 1,0,0,1 });
                        renderer.DrawLine(Vector3::up * 1000, Vector3::down * 1000, { 0,1,0,1 });
                        renderer.DrawLine(Vector3::forward * 1000, Vector3::back * 1000, { 0,0,1,1 });

                        {
                            Frustum mainCameraFrustum = Frustum(player.GetViewMatrix() * camera.GetProjectionMatrix());

                            Matrix4x4 lightViewMatrix = Matrix4x4::MakeAffineInverse(Matrix4x4::MakeLookRotation(light.direction, Vector3::unitX), -light.direction * 5000.0f);
                            Matrix4x4 lightProjMatrix = Matrix4x4::MakeOrthographicProjection(1024.0f, 1024.0f, 1.0f,10000.0f);


                            const uint32_t numShadowMaps = 3;
                            const float splitAreaTable[numShadowMaps + 1] = {
                                camera.GetNearClip(),
                                10.0f,
                                50.0f,
                               camera.GetFarClip()
                            };

                            Matrix4x4 lightViewProjMatrix = lightViewMatrix * lightProjMatrix;
                            DrawFrustum(Frustum(lightViewProjMatrix), renderer, { 0,0,0,1 });
                            Frustum shadowFrustums[numShadowMaps];
                            const Vector4 frustumColor[]{
                                {1.0f,0.0f,0.0f,1.0f},
                                {0.0f,1.0f,0.0f,1.0f},
                                {0.0f,0.0f,1.0f,1.0f}
                            };
                            for (uint32_t splitIndex = 0; splitIndex < numShadowMaps; ++splitIndex) {
                                AABB cropAABB;
                                Matrix4x4 projMatrix = Matrix4x4::MakePerspectiveProjection(camera.GetFovY(), camera.GetAspectRaito(), splitAreaTable[splitIndex], splitAreaTable[splitIndex + 1]);
                                Frustum frustum = Frustum(player.GetViewMatrix() * projMatrix);
                                DrawFrustum(frustum, renderer, frustumColor[splitIndex]);

                                for (int i = 0; i < Frustum::NumCorners; ++i) {
                                    cropAABB.Include(lightViewProjMatrix.ApplyTransformWDivide(frustum.GetCorner(static_cast<Frustum::CornerID>(i))));
                                }
                                Vector3 scale, offset;
                                scale.x = 2.0f / (cropAABB.max.x - cropAABB.min.x);
                                scale.y = 2.0f / (cropAABB.max.y - cropAABB.min.y);
                                scale.z = 1.0f;
                                offset.x = -0.5f * (cropAABB.max.x + cropAABB.min.x) * scale.x;
                                offset.y = -0.5f * (cropAABB.max.y + cropAABB.min.y) * scale.y;
                                offset.z = 0.0f;
                                Matrix4x4 cropMatrix{
                                    scale.x, 0.0f,0.0f,0.0f,
                                    0.0f, scale.y,0.0f,0.0f,
                                    0.0f, 0.0f,scale.z,0.0f,
                                    offset.x, offset.y, offset.z, 1.0f };
                                shadowFrustums[splitIndex] = Frustum(lightViewProjMatrix * cropMatrix);
                                DrawFrustum(shadowFrustums[splitIndex], renderer, frustumColor[splitIndex]);

                                Vector3 v[8];
                                for (uint32_t i = 0; i < 8; ++i) {
                                   v[i] = lightViewMatrix.ApplyTransformWDivide(frustum.GetCorner(static_cast<Frustum::CornerID>(i)));
                                   v;
                                }
                                v[0] = v[0];
                            }

                            //// Build a matrix for cropping light's projection    
                            //// Given vectors are in light's clip space 
                            //Matrix Light::CalculateCropMatrix(Frustum splitFrustum) {
                            //    Matrix lightViewProjMatrix = viewMatrix * projMatrix;
                            //    // Find boundaries in light's clip space   
                            //    BoundingBox cropBB = CreateAABB(splitFrustum.AABB, lightViewProjMatrix);
                            //    // Use default near-plane value   
                            //    cropBB.min.z = 0.0f;
                            //    // Create the crop matrix   
                            //    float scaleX, scaleY, scaleZ;
                            //    float offsetX, offsetY, offsetZ;
                            //    scaleX = 2.0f / (cropBB.max.x - cropBB.min.x);
                            //    scaleY = 2.0f / (cropBB.max.y - cropBB.min.y);
                            //    offsetX = -0.5f * (cropBB.max.x + cropBB.min.x) * scaleX;
                            //    offsetY = -0.5f * (cropBB.max.y + cropBB.min.y) * scaleY;
                            //    scaleZ = 1.0f / (cropBB.max.z - cropBB.min.z);
                            //    offsetZ = -cropBB.min.z * scaleZ;
                            //    return Matrix(scaleX, 0.0f, 0.0f, 0.0f,
                            //        0.0f, scaleY, 0.0f, 0.0f,
                            //        0.0f, 0.0f, scaleZ, 0.0f,
                            //        offsetX, offsetY, offsetZ, 1.0f);
                            //}

                           // DrawFrustum(mainCameraFrustum, renderer, Vector4(1.0f));

                        }

                        ImGui::Begin("Setting");
                        ImGui::Checkbox("Show aabb", &showAABB);
                        ImGui::Checkbox("Debug camera", &useDebugCamera);

                        if (ImGui::TreeNodeEx("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::Ex::DragDegree3("Direction rotate", lightRotate);
                            light.direction = Quaternion::MakeFromEulerAngle(lightRotate) * Vector3::unitZ;
                            ImGui::Ex::TextVector3("Direction", light.direction);
                            ImGui::ColorEdit4("Color", &light.color.x);
                            ImGui::DragFloat("Intensity", &light.intensity, 0.01f);
                            ImGui::TreePop();
                        }

                        ImGui::End();

                        //player.ImGuiEdit();

                        renderer.EndRendering();
                    }
                }
            }
        }
        renderer.Finalize();

    }
    return 0;
}

void World::Initialize(CollisionManager& collisionManager) {
    std::unique_ptr<BoxCollider> box;
    std::unique_ptr<SphereCollider> sphere;
    std::unique_ptr<CapsuleCollider> capsule;


    box = std::make_unique<BoxCollider>();
    box->isStatic = true;
    box->transform.translate = { 0.0f,-0.5f,0.0f };
    box->size = { 100.0f,1.0f,100.0f };
    box->color = { 0.1f,0.3f,0.2f,1.0f };
    box->transform.UpdateWorldMatrix();
    box->UpdateAABB();
    collisionManager.AddCollider(box.get());
    boxObjects_.emplace_back(std::move(box));

    box = std::make_unique<BoxCollider>();
    box->isStatic = true;
    box->transform.translate = { 0.0f,0.5f,5.0f };
    box->size = { 3.0f,1.0f,3.0f };
    box->color = { 0.0f,0.0f,0.5f,1.0f };
    box->transform.UpdateWorldMatrix();
    box->UpdateAABB();
    collisionManager.AddCollider(box.get());
    boxObjects_.emplace_back(std::move(box));

    box = std::make_unique<BoxCollider>();
    box->isStatic = true;
    box->transform.translate = { 7.0f,1.5f,5.0f };
    box->size = { 3.0f,1.0f,3.0f };
    box->color = { 0.0f,0.0f,0.5f,1.0f };
    box->transform.UpdateWorldMatrix();
    box->UpdateAABB();
    collisionManager.AddCollider(box.get());
    boxObjects_.emplace_back(std::move(box));

    box = std::make_unique<BoxCollider>();
    box->isStatic = true;
    box->transform.translate = { 14.0f,2.5f,3.5f };
    box->size = { 3.0f,1.0f,6.0f };
    box->color = { 0.0f,0.0f,0.5f,1.0f };
    box->transform.UpdateWorldMatrix();
    box->UpdateAABB();
    collisionManager.AddCollider(box.get());
    boxObjects_.emplace_back(std::move(box));

    box = std::make_unique<BoxCollider>();
    box->isStatic = true;
    box->transform.translate = { -7.0f,1.5f,0.0f };
    box->transform.rotate = Quaternion::MakeFromAngleAxis(20.0f * Math::ToRadian, Vector3::unitX);
    box->size = { 5.0f,1.0f,15.0f };
    box->color = { 0.0f,0.4f,0.4f,1.0f };
    box->transform.UpdateWorldMatrix();
    box->UpdateAABB();
    collisionManager.AddCollider(box.get());
    boxObjects_.emplace_back(std::move(box));

    box = std::make_unique<BoxCollider>();
    box->isStatic = true;
    box->transform.translate = { -7.0f,4.05f,-10.37f };
    box->size = { 5.0f,1.0f,7.0f };
    box->color = { 0.0f,0.4f,0.4f,1.0f };
    box->transform.UpdateWorldMatrix();
    box->UpdateAABB();
    collisionManager.AddCollider(box.get());
    boxObjects_.emplace_back(std::move(box));

    sphere = std::make_unique<SphereCollider>();
    sphere->isStatic = true;
    sphere->transform.translate = { 14.0f, 2.0f, -4.0f };
    sphere->radius = 2.0f;
    sphere->color = { 1.0f,0.0f,0.0f,1.0f };
    sphere->transform.UpdateWorldMatrix();
    sphere->UpdateAABB();
    collisionManager.AddCollider(sphere.get());
    sphereObjects_.emplace_back(std::move(sphere));

    capsule = std::make_unique<CapsuleCollider>();
    capsule->isStatic = true;
    capsule->transform.translate = { 7.0f, 0.0f, -10.0f };
    capsule->start = { 3.0f,0.0f,0.0f };
    capsule->end = { -3.0f,0.0f,0.0f };
    capsule->radius = 3.0f;
    capsule->color = { 1.0f,0.0f,0.0f,1.0f };
    capsule->transform.UpdateWorldMatrix();
    capsule->UpdateAABB();
    collisionManager.AddCollider(capsule.get());
    capsuleObjects_.emplace_back(std::move(capsule));
}

void World::Draw(Renderer& renderer, bool showAABB) {
    for (auto& object : boxObjects_) {
        DrawBox(*object, renderer, object->color);
        if (showAABB) {
            DrawAABB(object->GetAABB(), renderer, { 0.0f,0.0f,0.0f,1.0f });
        }
    }
    for (auto& object : sphereObjects_) {
        DrawSphere(*object, renderer, object->color);
        if (showAABB) {
            DrawAABB(object->GetAABB(), renderer, { 0.0f,0.0f,0.0f,1.0f });
        }
    }
    for (auto& object : capsuleObjects_) {
        DrawCapsule(*object, renderer, object->color);
        if (showAABB) {
            DrawAABB(object->GetAABB(), renderer, { 0.0f,0.0f,0.0f,1.0f });
        }
    }
}
