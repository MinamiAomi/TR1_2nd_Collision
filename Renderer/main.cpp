#include <Windows.h>

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
    Vector3 rotate;
    Vector3 translate;
};

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

    Renderer renderer;
    renderer.Initailize(L"RendererTest", 1280, 720);
    std::size_t plane = 0;
    std::size_t box = 0;
    {
        std::vector<Vertex> vertices = {
            { { -1.0f, 0.0f, -1.0f }, {0.0f,1.0f,0.0f} },
            { { -1.0f, 0.0f,  1.0f }, {0.0f,1.0f,0.0f} },
            { {  1.0f, 0.0f, -1.0f }, {0.0f,1.0f,0.0f} },
            { {  1.0f, 0.0f,  1.0f }, {0.0f,1.0f,0.0f} } };
        std::vector<uint16_t> indices = {
            0,1,2,
            1,3,2 };
        plane = renderer.RegisterMesh(vertices, indices);
    }
    //    {
    //
    //        const int kSurfaceCount = 6;
    //        const int kVertexCount = 24;
    //        const int kIndexCount = 36;
    //        std::vector<Vertex> vertices(kVertexCount);
    //        std::vector<uint16_t> indices(kIndexCount);
    //        // 座標
    //        {
    //            enum {
    //                LTN,    // 左上前
    //                LBN,    // 左下前
    //                RTN,    // 右上前
    //                RBN,    // 右下前
    //                LTF,    // 左上奥
    //                LBF,    // 左下奥
    //                RTF,    // 右上奥
    //                RBF,    // 右下奥
    //            };
    //
    //            Vector3 position[8] = {
    //                { -0.5f,  0.5f, -0.5f }, // 左上前
    //                { -0.5f, -0.5f, -0.5f }, // 左下前
    //                {  0.5f,  0.5f, -0.5f }, // 右上前
    //                {  0.5f, -0.5f, -0.5f }, // 右下前
    //                { -0.5f,  0.5f,  0.5f }, // 左上奥
    //                { -0.5f, -0.5f,  0.5f }, // 左下奥
    //                {  0.5f,  0.5f,  0.5f }, // 右上奥
    //                {  0.5f, -0.5f,  0.5f }, // 右下奥 
    //            };
    //
    //            // 前面
    //            vertices[0].position = position[LTN];
    //            vertices[1].position = position[LBN];
    //            vertices[2].position = position[RTN];
    //            vertices[3].position = position[RBN];
    //            // 後面
    //            vertices[4].position = position[RTF];
    //            vertices[5].position = position[RBF];
    //            vertices[6].position = position[LTF];
    //            vertices[7].position = position[LBF];
    //            // 右面
    //            vertices[8].position = position[RTN];
    //            vertices[9].position = position[RBN];
    //            vertices[10].position = position[RTF];
    //            vertices[11].position = position[RBF];
    //            // 左面
    //            vertices[12].position = position[LTF];
    //            vertices[13].position = position[LBF];
    //            vertices[14].position = position[LTN];
    //            vertices[15].position = position[LBN];
    //            // 上面
    //            vertices[16].position = position[LTF];
    //            vertices[17].position = position[LTN];
    //            vertices[18].position = position[RTF];
    //            vertices[19].position = position[RTN];
    //            // 下面
    //            vertices[20].position = position[LBN];
    //            vertices[21].position = position[LBF];
    //            vertices[22].position = position[RBN];
    //            vertices[23].position = position[RBF];
    //        }
    //
    //        // 法線
    ////        {
    ////            Vector3 normal[6] = {
    ////                -Vector3::unitZ, // 前面
    ////                 Vector3::unitZ, // 後面
    ////                 Vector3::unitX, // 右面
    ////                -Vector3::unitX, // 左面
    ////                 Vector3::unitY, // 上面
    ////                -Vector3::unitY, // 下面
    ////            };
    ////
    ////            for (size_t i = 0; i < kSurfaceCount; i++) {
    ////                size_t j = i * 4;
    ////                vertices[j + 0].normal = normal[i];
    ////                vertices[j + 1].normal = normal[i];
    ////                vertices[j + 2].normal = normal[i];
    ////                vertices[j + 3].normal = normal[i];
    ////            }
    ////        }
    //
    //        // インデックス
    //        {
    //            uint16_t i = 0;
    //            for (uint16_t j = 0; j < kSurfaceCount; j++) {
    //                uint16_t k = j * 4;
    //
    //                indices[i++] = k + 1;
    //                indices[i++] = k;
    //                indices[i++] = k + 2;
    //                indices[i++] = k + 1;
    //                indices[i++] = k + 2;
    //                indices[i++] = k + 3;
    //            }
    //        }
    //
    //        {
    //            std::vector<std::vector<Vector3>> surfaceNormals(vertices.size());
    //            for (size_t i = 0; i < indices.size(); i += 3) {
    //                auto index0 = indices[i + 0];
    //                auto index1 = indices[i + 1];
    //                auto index2 = indices[i + 2];
    //
    //                auto& vertex0 = vertices[index0];
    //                auto& vertex1 = vertices[index1];
    //                auto& vertex2 = vertices[index2];
    //
    //                auto normal = Vector3::Cross(vertex1.position - vertex0.position, vertex2.position - vertex1.position);
    //                normal = normal.Normalized();
    //
    //                //vertex0.normal = normal;
    //                //vertex1.normal = normal;
    //                //vertex2.normal = normal;
    //                surfaceNormals[index0].emplace_back(normal);
    //                surfaceNormals[index1].emplace_back(normal);
    //                surfaceNormals[index2].emplace_back(normal);
    //            }
    //            auto vertexNormals = surfaceNormals.begin();
    //            std::for_each(vertices.begin(), vertices.end(), 
    //                [&](auto& itr) {
    //                    Vector3 sumNormal;
    //                    for (auto& normal : *vertexNormals) {
    //                        sumNormal += normal;
    //                    }
    //                    itr.normal = sumNormal / static_cast<float>(vertexNormals->size());
    //                    itr.normal = itr.normal;
    //                    vertexNormals++;
    //                });
    //        }
    //
    //        box = renderer.RegisterMesh(vertices, indices);
    //    }
    {
        std::vector<Vector3> vertices = {
            { -0.5f,  0.5f, -0.5f },/* 左上前 */
            { -0.5f, -0.5f, -0.5f },/* 左下前 */
            {  0.5f,  0.5f, -0.5f },/* 右上前 */
            {  0.5f, -0.5f, -0.5f },/* 右下前 */
            { -0.5f,  0.5f,  0.5f },/* 左上奥 */
            { -0.5f, -0.5f,  0.5f },/* 左下奥 */
            {  0.5f,  0.5f,  0.5f },/* 右上奥 */
            {  0.5f, -0.5f,  0.5f } /* 右下奥 */ };
        std::vector<uint16_t> indices = {
            // 前
            1,0,2,
            1,2,3,
            // 左
            5,4,0,
            5,0,1,
            // 上
            0,4,6,
            0,6,2,
            // 右
            3,2,6,
            3,6,7,
            // 下
            5,1,3,
            5,3,7,
            // 後
            7,6,4,
            7,4,5 };





        {
            std::vector<std::vector<Vector3>> surfaceNormals(vertices.size());
            for (size_t i = 0; i < indices.size(); i += 3) {
                auto index0 = indices[i + 0];
                auto index1 = indices[i + 1];
                auto index2 = indices[i + 2];

                auto& vertex0 = vertices[index0];
                auto& vertex1 = vertices[index1];
                auto& vertex2 = vertices[index2];

                auto normal = Vector3::Cross(vertex1.position - vertex0.position, vertex2.position - vertex1.position);
                normal = normal.Normalized();

                //vertex0.normal = normal;
                //vertex1.normal = normal;
                //vertex2.normal = normal;
                surfaceNormals[index0].emplace_back(normal);
                surfaceNormals[index1].emplace_back(normal);
                surfaceNormals[index2].emplace_back(normal);
            }
            auto vertexNormals = surfaceNormals.begin();
            std::for_each(vertices.begin(), vertices.end(),
                [&](auto& itr) {
                    Vector3 sumNormal;
                    for (auto& normal : *vertexNormals) {
                        sumNormal += normal;
                    }
                    itr.normal = sumNormal / static_cast<float>(vertexNormals->size());
                    itr.normal = itr.normal;
                    vertexNormals++;
                });
        }

        box = renderer.RegisterMesh(vertices, indices);
    }

    Vector3 translate{ 0.0f,0.0f,0.0f };
    float speed = 0.01f;
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

                translate.y += speed;
                if (translate.y > 2 || translate.y < -2) {
                    speed = -speed;
                }
                renderer.DrawObject(plane, Vector3{ 0.4f }, Quaternion::identity, translate, Vector4{ 1.0f });
                renderer.DrawObject(plane, Vector3{ 0.2f }, Quaternion::MakeForXAxis(-45.0f * Math::ToRadian) * Quaternion::MakeForYAxis(45.0f * Math::ToRadian), { 1.0f,-2.0f,0.0f }, Vector4{ 0.0f, 0.0f, 1.0f,1.0f });
                renderer.DrawObject(plane, Vector3{ 0.7f }, Quaternion::identity, { 2.0f,0.5f,1.0f }, Vector4{ 1.0f, 1.0f,0.0f,1.0f });

                renderer.DrawObject(box, Vector3{ 1.0f }, Quaternion::identity, translate + Vector3{ 1.0f,0.0f,0.0f }, Vector4{ 1.0f });
                renderer.DrawObject(box, Vector3{ 0.3f }, Quaternion::MakeForXAxis(45.0f * Math::ToRadian), { -2.0f,1.0f,0.0f }, Vector4{ 0.0f, 0.0f, 1.0f,1.0f });
                renderer.DrawObject(box, Vector3{ 1.0f }, Quaternion::identity, { 1.0f,0.5f,1.0f }, Vector4{ 1.0f, 1.0f,0.0f,1.0f });



                renderer.EndRendering();
            }
        }
    }

    renderer.Finalize();

    return 0;
}
