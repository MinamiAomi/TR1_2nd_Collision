#include <Windows.h>

#include "Externals/ImGui/imgui.h"

#include "Math/MathUtils.hpp"
#include "Renderer.hpp"
#include "ShaderUtils.hpp"
#include "Utils.hpp"

#include "GameObject.hpp"

#include "Scene.hpp"

#include "HierarchyView.hpp"
#include "InspectorView.hpp"

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

     
    Scene scene;
    scene.SetName("Scene");
    auto& obj1 = scene.AddGameObject("obj1");
    auto& obj2 = scene.AddGameObject("obj2");
    auto& obj3 = scene.AddGameObject("obj3");
    obj3.SetParent(&obj1);
    obj2;

    HierarchyView hierarchyView;
    hierarchyView.SetScene(&scene);
    InspectorView inspectorView;
    hierarchyView.SetInspectorView(&inspectorView);
    

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

            /*    obj1.transform.UpdateWorldMatrix();
                obj2.transform.UpdateWorldMatrix();

                renderer.DrawBox(obj1.transform.GetWorldMatrix(), {0.6f,0.6f,0.6f,1.0f}, DrawMode::kObject);
                renderer.DrawBox(obj2.transform.GetWorldMatrix(), { 1.0f,0.0f,1.0f,0.4f }, DrawMode::kObject);*/

                ImGui::SetNextWindowPos({0,0},ImGuiCond_Once);
                ImGui::SetNextWindowSize({300,100},ImGuiCond_Once);
                ImGui::Begin("Window");
                bool is = hierarchyView.IsVisible();
                ImGui::Checkbox("HierarchyView", &is);
                hierarchyView.SetIsVisible(is);
                is = inspectorView.IsVisible();
                ImGui::Checkbox("InspectorView", &is);
                inspectorView.SetIsVisible(is);
                ImGui::End();

                hierarchyView.Show();
                inspectorView.Show();

                //ImGui::ShowDemoWindow();

                renderer.EndRendering();
            }
        }
    }

    renderer.Finalize();

    return 0;
}
