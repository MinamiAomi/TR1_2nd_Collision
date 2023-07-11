#include <Windows.h>

#include "Externals/ImGui/imgui.h"

#include "Math/MathUtils.hpp"
#include "Renderer.hpp"
#include "ShaderUtils.hpp"
#include "Utils.hpp"

#include "GameObject.hpp"

// 定数
constexpr uint32_t kSwapChainBufferCount = 2;
// クライアント領域サイズ
const uint32_t kClientWidth = 1280;
const uint32_t kClientHeight = 720;

class Test : public Behavior {
public:
    Test(GameObject* g) : Behavior(g) {}

    void Initalize() override {}
    void Update() override {}

};

class Test2 : public Component {
public:
    Test2(GameObject* g) : Component(g) {}


};

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

    Renderer renderer;
    renderer.Initailize(L"RendererTest", 1280, 720);

    GameObject obj1;
    obj1.transform.scale = { 1.0f,1.0f,1.0f };
    obj1.transform.rotate = Quaternion::identity;
    obj1.transform.translate = { 2.0f,0.0f,0.0f };
    obj1.AddComponent<Test>();
    obj1.AddComponent<Test2>();
    obj1.GetComponent<Test>();
    obj1.GetComponent<Test2>();
    obj1.RemoveComponent<Test>();
    obj1.RemoveComponent<Test2>();

    GameObject obj2;
    obj2.transform.scale = { 1.0f,1.0f,1.0f };
    obj2.transform.rotate = Quaternion::identity;
    obj2.transform.translate = { 0.0f,1.0f,0.0f };
    obj2.SetParent(&obj1);

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
                renderer.StartRendering();

                obj1.transform.UpdateWorldMatrix();
                obj2.transform.UpdateWorldMatrix();

                renderer.DrawBox(obj1.transform.GetWorldMatrix(), {0.6f,0.6f,0.6f,1.0f}, DrawMode::kObject);
                renderer.DrawBox(obj2.transform.GetWorldMatrix(), { 1.0f,0.0f,1.0f,0.4f }, DrawMode::kObject);

                ImGui::SetNextWindowPos({0,0},ImGuiCond_Once);
                ImGui::SetNextWindowSize({300,100},ImGuiCond_Once);
                ImGui::Begin("Window");
                ImGui::End();

                

                renderer.EndRendering();
            }
        }
    }

    renderer.Finalize();

    return 0;
}
