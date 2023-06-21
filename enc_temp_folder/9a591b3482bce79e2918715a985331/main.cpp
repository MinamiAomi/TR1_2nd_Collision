#include <Windows.h>

#include "Externals/ImGui/imgui.h"

#include "Math/MathUtils.hpp"
#include "Renderer.hpp"
#include "ShaderUtils.hpp"
// 定数
constexpr uint32_t kSwapChainBufferCount = 2;
// クライアント領域サイズ
const uint32_t kClientWidth = 1280;
const uint32_t kClientHeight = 720;

struct Transform {
    Vector3 scale;
    Quaternion rotate;
    Vector3 translate;

    Matrix4x4 MakeWorldMatrix() {
        return Matrix4x4::MakeAffineTransform(scale, rotate, translate);
    }
};

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

    int a = MessageBoxA(nullptr, "Rikihisa ha baka", "OK cancel", MB_OKCANCEL);
    if (a != 0) {
        MessageBoxA(nullptr, "0", "", 0);
    }
    else {
        MessageBoxA(nullptr, "!0", "", 0);

    }

    Renderer renderer;
    renderer.Initailize(L"RendererTest", 1280, 720);

    Transform transform[6];

    transform[0].scale = Vector3::one;
    transform[0].rotate = Quaternion::identity;
    transform[0].translate = Vector3::unitX;

    transform[1].scale = Vector3::one;
    transform[1].rotate = Quaternion::identity;
    transform[1].translate = { 1.0f,0.0f,-1.0f };

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

                renderer.DrawBox(transform[0].MakeWorldMatrix(), { 0.6f,0.6f,0.6f,1.0f }, DrawMode::kObject);
                renderer.DrawBox(transform[1].MakeWorldMatrix(), { 1.0f,0.0f,1.0f,0.4f }, DrawMode::kObject);

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
