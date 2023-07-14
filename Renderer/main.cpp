#include <Windows.h>

#include <stack>

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

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

    Renderer renderer;
    renderer.Initailize(L"RendererTest", 1280, 720);


    Scene scene;
    scene.SetName("Scene");
    for (size_t i = 0; i < 10; ++i) {
        scene.AddGameObject(("obj" + std::to_string(i)).c_str());
    }

    scene.GetGameObject(0).transform.scale = { 10.0f,1.0f,10.0f };
    scene.GetGameObject(0).transform.translate = { 0.0f,-1.0f,0.0f };
    

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

                for (auto& iter : scene.GetGameObjects()) {
                    std::stack<GameObject*> stack;
                    stack.push(iter.get());
                    while (!stack.empty()) {
                        GameObject* o = stack.top();
                        stack.pop();
                        o->transform.UpdateWorldMatrix();
                        for (auto c : o->GetChildren()) {
                            stack.push(c);
                        }
                    }
                }
                for (auto& o : scene.GetGameObjects()) {
                    renderer.DrawBox(o->transform.GetWorldMatrix(), { 1.0f,1.0f,1.0f,1.0f }, DrawMode::kObject);
                }

                ImGui::SetNextWindowPos({ 0,0 }, ImGuiCond_Once);
                ImGui::SetNextWindowSize({ 300,100 }, ImGuiCond_Once);
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

                renderer.EndRendering();
            }
        }
    }

    renderer.Finalize();

    return 0;
}
