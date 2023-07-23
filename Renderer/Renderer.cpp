#include "Renderer.hpp"

#include <Windows.h>

#include <cassert>
#include <cstdint>

#include "Externals/ImGui/imgui.h"
#include "Externals/ImGui/imgui_impl_dx12.h"
#include "Externals/ImGui/imgui_impl_win32.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {
   
    // ウィンドウプロシージャ
    LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) { return true; }
        // メッセージに対してゲーム固有の処理を行う
        switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    void CalcNormals(
        const std::vector<Vector3>& positions,
        const std::vector<std::uint16_t>& indices,
        std::vector<Renderer::Vertex>* smooth_vertices) {
        assert(smooth_vertices);
        smooth_vertices->resize(positions.size());

        std::vector<std::vector<Vector3>> surface_normals(positions.size());
        for (size_t i = 0; i < indices.size(); i += 3) {
            auto i0 = indices[i + 0];
            auto i1 = indices[i + 1];
            auto i2 = indices[i + 2];

            auto& pos0 = positions[i0];
            auto& pos1 = positions[i1];
            auto& pos2 = positions[i2];

            auto normal = Vector3::Cross(pos1 - pos0, pos2 - pos1);
            normal = normal.Normalized();

            surface_normals[i0].emplace_back(normal);
            surface_normals[i1].emplace_back(normal);
            surface_normals[i2].emplace_back(normal);
        }

        auto position = positions.begin();
        auto smooth_vertex = smooth_vertices->begin();
        auto vertex_normals = surface_normals.begin();
        for (; smooth_vertex != smooth_vertices->end(); position++, smooth_vertex++, vertex_normals++) {
            Vector3 sum_normal;
            for (auto& normal : *vertex_normals) {
                sum_normal += normal;
            }
            sum_normal = sum_normal / static_cast<float>(vertex_normals->size());
            smooth_vertex->position = *position;
            smooth_vertex->normal = sum_normal;
        }
    }
}

void Renderer::InitializeWin() {
    // ウィンドウクラスを生成
    WNDCLASS wc{};
    wc.lpfnWndProc = WindowProc;	// ウィンドウプロシージャ
    wc.lpszClassName = L"CG2WindowClass";	// ウィンドウクラス名
    wc.hInstance = GetModuleHandle(nullptr);	// インスタンスハンドル
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);	// カーソル
    RegisterClass(&wc);	// ウィンドウクラスを登録

    // ウィンドウサイズを表す構造体にクライアント領域を入れる
    RECT wrc{ 0,0,static_cast<LONG>(windowWidth_),static_cast<LONG>(windowHeight_) };
    // クライアント領域を元に実際のサイズにwrcを変更してもらう
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    // ウィンドウの生成
    hWnd_ = CreateWindow(
        wc.lpszClassName,		// 利用するクラス名
        L"Collision",				// タイトルバーの文字
        WS_OVERLAPPEDWINDOW,	// よく見るウィンドウスタイル
        CW_USEDEFAULT,			// 表示X座標（WindowsOSに任せる）
        CW_USEDEFAULT,			// 表示Y座標（WindowsOSに任せる）
        wrc.right - wrc.left,	// ウィンドウ横幅
        wrc.bottom - wrc.top,	// ウィンドウ縦幅
        nullptr,				// 親ウィンドウハンドル
        nullptr,				// メニューハンドル
        wc.hInstance,			// インスタンスハンドル
        nullptr);				// オプション
}

void Renderer::InitializeDX12Core() {
    device_ = std::make_unique<CG::DX12::Device>();
    commandQueue_ = std::make_unique<CG::DX12::CommandQueue>();
    for (auto& commandList : commandLists_) {
        commandList = std::make_unique<CG::DX12::CommandList>();
    }
    fence_ = std::make_unique<CG::DX12::Fence>();

    rtvDescriptorHeap_ = std::make_unique<CG::DX12::DescriptorHeap>();
    dsvDescriptorHeap_ = std::make_unique<CG::DX12::DescriptorHeap>();
    srvDescriptorHeap_ = std::make_unique<CG::DX12::DescriptorHeap>();

    swapChain_ = std::make_unique<CG::DX12::SwapChain>();
    depthStencilResource_ = std::make_unique<CG::DX12::DepthStencilResource>();

    device_->Initialize();
    commandQueue_->Initialize(*device_, D3D12_COMMAND_LIST_TYPE_DIRECT);
    for (auto& commandList : commandLists_) {
        commandList->Initialize(*device_, *commandQueue_);
    }
    fence_->Initialize(*device_);

    rtvDescriptorHeap_->Initialize(*device_, 2, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsvDescriptorHeap_->Initialize(*device_, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    srvDescriptorHeap_->Initialize(*device_, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    swapChain_->Initialize(hWnd_, *device_, *commandQueue_, *rtvDescriptorHeap_, windowWidth_, windowHeight_);
    depthStencilResource_->resource.InitializeForTexture2D(
        *device_,
        windowWidth_, windowHeight_, 1, 1,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        CG::DX12::Resource::State::DepthWrite,
        CG::DX12::Resource::HeapType::Default,
        CG::DX12::Resource::Flag::AllowDepthStencil);
    depthStencilResource_->view.Initialize(*device_, depthStencilResource_->resource, dsvDescriptorHeap_->Allocate());

    viewport_.Width = static_cast<float>(windowWidth_);
    viewport_.Height = static_cast<float>(windowHeight_);
    viewport_.TopLeftX = 0.0f;
    viewport_.TopLeftY = 0.0f;
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    scissorRect_.left = 0;
    scissorRect_.right = static_cast<LONG>(windowWidth_);
    scissorRect_.top = 0;
    scissorRect_.bottom = static_cast<LONG>(windowHeight_);


    imguiDescriptor_ = srvDescriptorHeap_->Allocate();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hWnd_);
    ImGui_ImplDX12_Init(
        device_->GetDevice().Get(),
        CG::DX12::SwapChain::kBackBufferCount,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        srvDescriptorHeap_->GetDescriptorHeap().Get(),
        imguiDescriptor_.GetCPUHandle(),
        imguiDescriptor_.GetGPUHandle());

}

void Renderer::InitializePSO() {
    rootSignature_ = std::make_unique<CG::DX12::RootSignature>();
    objectPipelineState_ = std::make_unique<CG::DX12::PipelineState>();
    wireFramePipelineState_ = std::make_unique<CG::DX12::PipelineState>();
    linePipelineState_ = std::make_unique<CG::DX12::PipelineState>();

    CG::DX12::ShaderCompiler shaderCompiler;
    shaderCompiler.Initialize();

    CG::DX12::RootSignatureDesc rootSignatureDesc;
    rootSignatureDesc.AddDescriptor(CG::DX12::DescriptorType::CBV, 0, CG::DX12::ShaderVisibility::All);
    rootSignatureDesc.AddFlag(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    rootSignature_->Initialize(*device_, rootSignatureDesc);

    {
        CG::DX12::Shader vertexShader = shaderCompiler.Compile(L"object_vs.hlsl", L"vs_6_0");
        CG::DX12::Shader pixelShader = shaderCompiler.Compile(L"object_ps.hlsl", L"ps_6_0");

        CG::DX12::GraphicsPipelineStateDesc pipelineStateDesc;
        pipelineStateDesc.SetRootSignature(*rootSignature_);
        pipelineStateDesc.SetVertexShader(vertexShader);
        pipelineStateDesc.SetPixelShader(pixelShader);
        pipelineStateDesc.AddInputElementVertex("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0);
        pipelineStateDesc.AddInputElementVertex("NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0);
        pipelineStateDesc.AddInputElementInstance("MATRIX", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 1);
        pipelineStateDesc.AddInputElementInstance("MATRIX", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 1);
        pipelineStateDesc.AddInputElementInstance("MATRIX", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 1);
        pipelineStateDesc.AddInputElementInstance("COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 1);
        pipelineStateDesc.AddRenderTargetState(CG::DX12::BlendMode::Normal, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        pipelineStateDesc.SetDepthState(DXGI_FORMAT_D24_UNORM_S8_UINT);
        pipelineStateDesc.SetRasterizerState(CG::DX12::CullMode::Back, CG::DX12::FillMode::Solid);
        pipelineStateDesc.SetPrimitiveTopologyType(CG::DX12::PrimitiveTopology::Triangle);
        pipelineStateDesc.SetSampleState();
        objectPipelineState_->Initialize(*device_, pipelineStateDesc);
    }
    {
        CG::DX12::Shader vertexShader = shaderCompiler.Compile(L"wire_frame_vs.hlsl", L"vs_6_0");
        CG::DX12::Shader pixelShader = shaderCompiler.Compile(L"wire_frame_ps.hlsl", L"ps_6_0");

        CG::DX12::GraphicsPipelineStateDesc pipelineStateDesc;
        pipelineStateDesc.SetRootSignature(*rootSignature_);
        pipelineStateDesc.SetVertexShader(vertexShader);
        pipelineStateDesc.SetPixelShader(pixelShader);
        pipelineStateDesc.AddInputElementVertex("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0);
        pipelineStateDesc.AddInputElementVertex("NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0);
        pipelineStateDesc.AddInputElementInstance("MATRIX", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 1);
        pipelineStateDesc.AddInputElementInstance("MATRIX", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 1);
        pipelineStateDesc.AddInputElementInstance("MATRIX", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 1);
        pipelineStateDesc.AddInputElementInstance("COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 1);
        pipelineStateDesc.AddRenderTargetState(CG::DX12::BlendMode::Normal, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        pipelineStateDesc.SetDepthState(DXGI_FORMAT_D24_UNORM_S8_UINT);
        pipelineStateDesc.SetRasterizerState(CG::DX12::CullMode::None, CG::DX12::FillMode::WireFrame);
        pipelineStateDesc.SetPrimitiveTopologyType(CG::DX12::PrimitiveTopology::Triangle);
        pipelineStateDesc.SetSampleState();
        wireFramePipelineState_->Initialize(*device_, pipelineStateDesc);
    }
    {
        CG::DX12::Shader vertexShader = shaderCompiler.Compile(L"line_vs.hlsl", L"vs_6_0");
        CG::DX12::Shader pixelShader = shaderCompiler.Compile(L"line_ps.hlsl", L"ps_6_0");

        CG::DX12::GraphicsPipelineStateDesc pipelineStateDesc;
        pipelineStateDesc.SetRootSignature(*rootSignature_);
        pipelineStateDesc.SetVertexShader(vertexShader);
        pipelineStateDesc.SetPixelShader(pixelShader);
        pipelineStateDesc.AddInputElementVertex("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0);
        pipelineStateDesc.AddInputElementVertex("COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
        pipelineStateDesc.AddRenderTargetState(CG::DX12::BlendMode::Normal, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        pipelineStateDesc.SetDepthState(DXGI_FORMAT_D24_UNORM_S8_UINT);
        pipelineStateDesc.SetRasterizerState(CG::DX12::CullMode::None, CG::DX12::FillMode::Solid);
        pipelineStateDesc.SetPrimitiveTopologyType(CG::DX12::PrimitiveTopology::Line);
        pipelineStateDesc.SetSampleState();
        linePipelineState_->Initialize(*device_, pipelineStateDesc);
    }
}

void Renderer::InitializeBuffers() {
    size_t sceneBufferSize = (sizeof(Scene) + 0xFF) & ~0xFF;
    for (size_t i = 0; i < CG::DX12::SwapChain::kBackBufferCount; ++i) {
        instancingBuffers_[i] = std::make_unique<CG::DX12::DynamicBuffer>();
        lineBuffers_[i] = std::make_unique<CG::DX12::DynamicBuffer>();
        sceneBuffers_[i] = std::make_unique<CG::DX12::DynamicBuffer>();

        instancingBuffers_[i]->Initialize(*device_, sizeof(Instance) * kMaxInstanceCount);
        lineBuffers_[i]->Initialize(*device_, sizeof(LineVertex) * kMaxLineCount * 2);
        sceneBuffers_[i]->Initialize(*device_, sceneBufferSize);
    }
}

void Renderer::AddInstance(MeshHandle mesh, const Matrix4x4& worldMatrix, const Vector4& color, DrawMode drawMode) {
    assert(instanceCount_ < kMaxInstanceCount);
    Matrix4x4 matrix = worldMatrix.Transpose();
    Instance instance;
    for (size_t i = 0; i < 3; ++i) {
        instance.worldMatrix[i] = matrix.GetRow(i);
    }
    instance.color = color;
    switch (drawMode) {
    case Renderer::DrawMode::Object:
        objectInstancesMap[mesh.handle].emplace_back(instance);
        break;
    case Renderer::DrawMode::WireFrame:
        wireFrameInstancesMap[mesh.handle].emplace_back(instance);
        break;
    }
    ++instanceCount_;
}

void Renderer::AddLine(const Vector3& p0, const Vector3& p1, const Vector4& color) {
    assert(lineCount_ < kMaxLineCount);
    auto bbIndex = swapChain_->GetCurrentBackBufferIndex();
    LineVertex* dest = lineBuffers_[bbIndex]->GetDataBegin<LineVertex>() + lineCount_ * 2;
    dest->position = p0;
    dest->color = color;
    ++dest;
    dest->position = p1;
    dest->color = color;
    ++lineCount_;
}

void Renderer::DrawAllInstances() {
    auto bbIndex = swapChain_->GetCurrentBackBufferIndex();
    auto cmdList = commandLists_[swapChain_->GetCurrentBackBufferIndex()]->GetCommandList();
    auto& instancingBuffer = *instancingBuffers_[bbIndex];

    Instance* instanceCopyDest = instancingBuffer.GetDataBegin<Instance>();
    uint32_t instanceLocation = 0;

    D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[2]{};
    // インスタンス用バッファ
    vertexBufferViews[1].BufferLocation = instancingBuffer.GetResource().GetGPUVirtualAddress();
    vertexBufferViews[1].SizeInBytes = static_cast<uint32_t>(instancingBuffer.GetBufferSize());
    vertexBufferViews[1].StrideInBytes = static_cast<uint32_t>(sizeof(Instance));

    // ワイヤーフレームから描画
    cmdList->SetPipelineState(wireFramePipelineState_->GetPipelineState().Get());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (auto& wireFrameInstances : wireFrameInstancesMap) {
        auto& mesh = meshes_[wireFrameInstances.first];
        auto& instances = wireFrameInstances.second;
        uint32_t instanceCount = static_cast<uint32_t>(instances.size());

        size_t copySize = instanceCount * sizeof(instances[0]);
        memcpy(instanceCopyDest, instances.data(), copySize);
        instanceCopyDest += instances.size();

        vertexBufferViews[0] = mesh->vertexBufferView.GetView();
        cmdList->IASetVertexBuffers(0, 2, vertexBufferViews);
        cmdList->IASetIndexBuffer(&mesh->IndexBufferView.GetView());
        cmdList->DrawIndexedInstanced(mesh->IndexBufferView.GetIndexCount(), instanceCount, 0, 0, instanceLocation);
        instanceLocation += instanceCount;
    }


    cmdList->SetPipelineState(objectPipelineState_->GetPipelineState().Get());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (auto& objectInstances : objectInstancesMap) {
        auto& mesh = meshes_[objectInstances.first];
        auto& instances = objectInstances.second;
        uint32_t instanceCount = static_cast<uint32_t>(instances.size());

        size_t copySize = instanceCount * sizeof(instances[0]);
        memcpy(instanceCopyDest, instances.data(), copySize);
        instanceCopyDest += instances.size();

        vertexBufferViews[0] = mesh->vertexBufferView.GetView();
        cmdList->IASetVertexBuffers(0, 2, vertexBufferViews);
        cmdList->IASetIndexBuffer(&mesh->IndexBufferView.GetView());
        cmdList->DrawIndexedInstanced(mesh->IndexBufferView.GetIndexCount(), instanceCount, 0, 0, instanceLocation);
        instanceLocation += instanceCount;
    }

    assert(instanceLocation == instanceCount_);

    wireFrameInstancesMap.clear();
    objectInstancesMap.clear();
    instanceCount_ = 0;
}

void Renderer::DrawAllLines() {
    auto bbIndex = swapChain_->GetCurrentBackBufferIndex();
    auto cmdList = commandLists_[bbIndex]->GetCommandList();
    cmdList->SetPipelineState(linePipelineState_->GetPipelineState().Get());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    CG::DX12::VertexBufferView view;
    view.Initialize(lineBuffers_[bbIndex]->GetResource(), lineCount_ * 2 * sizeof(LineVertex), sizeof(LineVertex), lineCount_ * 2);
    cmdList->IASetVertexBuffers(0, 1, &view.GetView());
    cmdList->DrawInstanced(view.GetVertexCount(), 1, 0, 0);
    lineCount_ = 0;
}

void Renderer::Initialize(uint32_t windowWidth, uint32_t windowHeight) {
    windowWidth_ = windowWidth;
    windowHeight_ = windowHeight;

    InitializeWin();
    InitializeDX12Core();
    InitializePSO();
    InitializeBuffers();

    CreateMeshPlane();
    CreateMeshBox();
    CreateMeshSphere();
    CreateMeshCylinder();
    CreateMeshCone();

    ShowWindow(hWnd_, SW_SHOW);
    OutputDebugStringA("Initilize renderer !!");
}

void Renderer::StartRendering() {
    ImGui_ImplWin32_NewFrame();
    ImGui_ImplDX12_NewFrame();
    ImGui::NewFrame();

    auto bbIndex = swapChain_->GetCurrentBackBufferIndex();
    commandLists_[bbIndex]->Reset();
    auto cmdList = commandLists_[bbIndex]->GetCommandList();

    auto barrier = swapChain_->GetCurrentResource().TransitionBarrier(CG::DX12::Resource::State::RenderTarget);
    cmdList->ResourceBarrier(1, &barrier);
    auto rtvHandle = swapChain_->GetCurrentRenderTargetView().GetDescriptor().GetCPUHandle();
    auto dsvHandle = depthStencilResource_->view.GetDescriptor().GetCPUHandle();
    cmdList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
    float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
    cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    cmdList->RSSetViewports(1, &viewport_);
    cmdList->RSSetScissorRects(1, &scissorRect_);

    ID3D12DescriptorHeap* pHeaps = srvDescriptorHeap_->GetDescriptorHeap().Get();
    cmdList->SetDescriptorHeaps(1, &pHeaps);
}

void Renderer::EndRendering() {
    auto bbIndex = swapChain_->GetCurrentBackBufferIndex();
    auto cmdList = commandLists_[bbIndex]->GetCommandList();

    memcpy(sceneBuffers_[bbIndex]->GetDataBegin(), &scene_, sizeof(scene_));

    cmdList->SetGraphicsRootSignature(rootSignature_->GetRootSignature().Get());
    cmdList->SetGraphicsRootConstantBufferView(0, sceneBuffers_[bbIndex]->GetResource().GetGPUVirtualAddress());

    if (instanceCount_ > 0) {
        DrawAllInstances();
    }
    if (lineCount_ > 0) {
        DrawAllLines();
    }

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList.Get());

    auto barrier = swapChain_->GetCurrentResource().TransitionBarrier(CG::DX12::Resource::State::Present);
    cmdList->ResourceBarrier(1, &barrier);

    commandLists_[bbIndex]->Close();
    fence_->Wait();
    commandQueue_->ExcuteCommandList(*commandLists_[bbIndex]);
    swapChain_->Present(1);
    fence_->Signal(*commandQueue_);
}

void Renderer::Finalize() {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CloseWindow(hWnd_);
}

Renderer::MeshHandle Renderer::RegisterMesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices) {
    assert(nextMeshIndex < kMaxMeshCount);

    MeshHandle handle{ nextMeshIndex++ };
    assert(!meshes_[handle.handle]);

    Mesh& mesh = *(meshes_[handle.handle] = std::make_unique<Mesh>());

    size_t vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    mesh.vertexBuffer.InitializeForBuffer(*device_, vertexBufferSize);
    void* vertexDataBegin = nullptr;
    vertexDataBegin = mesh.vertexBuffer.Map();
    memcpy(vertexDataBegin, vertices.data(), vertexBufferSize);
    mesh.vertexBuffer.Unmap();
    mesh.vertexBufferView.Initialize(mesh.vertexBuffer, vertexBufferSize, sizeof(vertices[0]), vertices.size());

    size_t indexBufferSize = sizeof(indices[0]) * indices.size();
    mesh.indexBuffer.InitializeForBuffer(*device_, indexBufferSize);
    void* indexDataBegin = nullptr;
    indexDataBegin = mesh.indexBuffer.Map();
    memcpy(indexDataBegin, indices.data(), indexBufferSize);
    mesh.indexBuffer.Unmap();
    mesh.IndexBufferView.Initialize(mesh.indexBuffer, indexBufferSize, indices.size(), CG::DX12::IndexBufferView::Format::UInt16);

    return handle;
}

void Renderer::DrawPlane(const Matrix4x4& worldMatrix, const Vector4& color, DrawMode drawMode) {
    AddInstance(plane_, worldMatrix, color, drawMode);
}

void Renderer::DrawBox(const Matrix4x4& worldMatrix, const Vector4& color, DrawMode drawMode) {
    AddInstance(box_, worldMatrix, color, drawMode);
}

void Renderer::DrawSphere(const Matrix4x4& worldMatrix, const Vector4& color, DrawMode drawMode) {
    AddInstance(sphere_, worldMatrix, color, drawMode);
}

void Renderer::DrawCylinder(const Matrix4x4& worldMatrix, const Vector4& color, DrawMode drawMode) {
    AddInstance(cylinder_, worldMatrix, color, drawMode);
}

void Renderer::DrawCone(const Matrix4x4& worldMatrix, const Vector4& color, DrawMode drawMode) {
    AddInstance(cone_, worldMatrix, color, drawMode);
}

void Renderer::DrawMesh(MeshHandle meshHandle, const Matrix4x4& worldMatrix, const Vector4& color, DrawMode drawMode) {
    AddInstance(meshHandle, worldMatrix, color, drawMode);
}

void Renderer::DrawLine(const Vector3& v1, const Vector3& v2, const Vector4& color) {
    AddLine(v1, v2, color);
}

void Renderer::SetViewProjectionMatrix(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {
    scene_.viewProjectionMatrix = viewMatrix * projectionMatrix;
}

void Renderer::SetDirectionalLight(const Vector3& direction, const Vector4& color, float intensity) {
    scene_.lightDirection = direction;
    scene_.lightColor = color;
    scene_.lightIntensity = intensity;
}


void Renderer::CreateMeshPlane() {
    std::vector<Vertex> vertices = {
          {{ -0.5f, 0.0f, -0.5f }, {0.0f,1.0f,0.0f}},
          {{ -0.5f, 0.0f,  0.5f }, {0.0f,1.0f,0.0f}},
          {{  0.5f, 0.0f, -0.5f }, {0.0f,1.0f,0.0f}},
          {{  0.5f, 0.0f,  0.5f }, {0.0f,1.0f,0.0f}} };
    std::vector<uint16_t> indices = {
        0,1,2,
        1,3,2 };
    plane_ = RegisterMesh(vertices, indices);
}

void Renderer::CreateMeshBox() {
    const uint32_t kSurfaceCount = 6;
    const uint32_t kVertexCount = 24;
    const uint32_t kIndexCount = 36;
    std::vector<Vertex> vertices(kVertexCount);
    std::vector<uint16_t> indices(kIndexCount);

    enum { LTN, LBN, RTN, RBN, LTF, LBF, RTF, RBF, };

    Vector3 position[] = {
        { -0.5f,  0.5f, -0.5f }, // 左上前
        { -0.5f, -0.5f, -0.5f }, // 左下前
        {  0.5f,  0.5f, -0.5f }, // 右上前
        {  0.5f, -0.5f, -0.5f }, // 右下前

        { -0.5f,  0.5f,  0.5f }, // 左上奥
        { -0.5f, -0.5f,  0.5f }, // 左下奥
        {  0.5f,  0.5f,  0.5f }, // 右上奥
        {  0.5f, -0.5f,  0.5f }, // 右下奥 
    };

    // 前面
    vertices[0].position = position[LTN];
    vertices[1].position = position[LBN];
    vertices[2].position = position[RTN];
    vertices[3].position = position[RBN];
    // 後面
    vertices[4].position = position[RTF];
    vertices[5].position = position[RBF];
    vertices[6].position = position[LTF];
    vertices[7].position = position[LBF];
    // 右面
    vertices[8].position = position[RTN];
    vertices[9].position = position[RBN];
    vertices[10].position = position[RTF];
    vertices[11].position = position[RBF];
    // 左面
    vertices[12].position = position[LTF];
    vertices[13].position = position[LBF];
    vertices[14].position = position[LTN];
    vertices[15].position = position[LBN];
    // 上面
    vertices[16].position = position[LTF];
    vertices[17].position = position[LTN];
    vertices[18].position = position[RTF];
    vertices[19].position = position[RTN];
    // 下面
    vertices[20].position = position[LBN];
    vertices[21].position = position[LBF];
    vertices[22].position = position[RBN];
    vertices[23].position = position[RBF];

    // 法線
    Vector3 normal[] = {
        Vector3::back,
        Vector3::forward,
        Vector3::right,
        Vector3::left,
        Vector3::up,
        Vector3::down,
    };

    for (size_t i = 0; i < kSurfaceCount; i++) {
        size_t j = i * 4;
        vertices[j + 0].normal = normal[i];
        vertices[j + 1].normal = normal[i];
        vertices[j + 2].normal = normal[i];
        vertices[j + 3].normal = normal[i];
    }
    // インデックス
    for (uint16_t i = 0, j = 0; j < kSurfaceCount; j++) {
        uint16_t k = j * 4;

        indices[i++] = k + 1;
        indices[i++] = k;
        indices[i++] = k + 2;
        indices[i++] = k + 1;
        indices[i++] = k + 2;
        indices[i++] = k + 3;
    }

    box_ = RegisterMesh(vertices, indices);
}

void Renderer::CreateMeshSphere() {
    const uint32_t kSubdivision = 16;

    const float kLonEvery = Math::TwoPi / float(kSubdivision);
    const float kLatEvery = Math::Pi / float(kSubdivision);
    const size_t kLatVertexCount = kSubdivision + 1ull;
    const size_t kLonVertexCount = kSubdivision + 1ull;
    const size_t kVertexCount = kLonVertexCount * kLatVertexCount;
    const size_t kIndexCount = size_t(kSubdivision * kSubdivision) * 6;

    std::vector<Vertex> vertices(kVertexCount);
    std::vector<uint16_t> indices(kIndexCount);

    auto CalcPosition = [](float lat, float lon) {
        return Vector3{
            { std::cos(lat) * std::cos(lon) },
            { std::sin(lat) },
            { std::cos(lat) * std::sin(lon) }
        };
    };
    // 頂点
    for (size_t latIndex = 0; latIndex < kLonVertexCount; ++latIndex) {
        float lat = -Math::HalfPi + kLatEvery * latIndex;

        for (size_t lonIndex = 0; lonIndex < kLatVertexCount; ++lonIndex) {
            float lon = lonIndex * kLonEvery;

            size_t vertexIndex = latIndex * kLatVertexCount + lonIndex;
            vertices[vertexIndex].position = CalcPosition(lat, lon);
            vertices[vertexIndex].normal = static_cast<Vector3>(vertices[vertexIndex].position);
        }
    }
    // インデックス
    for (uint16_t i = 0; i < kSubdivision; ++i) {
        uint16_t y0 = i * kLatVertexCount;
        uint16_t y1 = (i + 1) * uint16_t(kLatVertexCount);

        for (uint16_t j = 0; j < kSubdivision; ++j) {
            uint16_t index0 = y0 + j;
            uint16_t index1 = y1 + j;
            uint16_t index2 = y0 + j + 1;
            uint16_t index3 = y1 + j + 1;

            uint16_t indexIndex = (i * kSubdivision + j) * 6;
            indices[indexIndex++] = index0;
            indices[indexIndex++] = index1;
            indices[indexIndex++] = index2;
            indices[indexIndex++] = index1;
            indices[indexIndex++] = index3;
            indices[indexIndex++] = index2;
        }
    }
    sphere_ = RegisterMesh(vertices, indices);
}

void Renderer::CreateMeshCylinder() {
    const size_t kSubdivision = 16;
    const float kAngleEvery = Math::TwoPi / float(kSubdivision);
    // 天板中央 + 天板外周分割数 + 側面分割数 * 上下
    // 天板はフラット側面はスムーズ
    const size_t kVertexCount = (1 + kSubdivision + kSubdivision) * 2;
    const size_t kIndexCount = kSubdivision * 4 * 3;
    const float kHeight = 1.0f;
    const float kHalfHeight = kHeight * 0.5f;

    std::vector<Vertex> vertices(kVertexCount);
    std::vector<uint16_t> indices(kIndexCount);

    // verticesレイアウト
    // 上天板中央 1, 上天板外周 分割数, 上側面　分割数, 下側面 分割数, 下天板 外周, 下天板中央 1　

    vertices[0] = { { 0.0f,kHalfHeight, 0.0f }, Vector3::up };
    for (size_t i = 0; i < kSubdivision; ++i) {
        float angle = kAngleEvery * i;
        float c = std::cos(angle), s = std::sin(angle);
        Vector3 upperPosition = { c,  kHalfHeight, s };
        Vector3 lowerPosition = { c, -kHalfHeight, s };
        Vector3 sideNormal = { c, 0.0f, s };
        vertices[i + kSubdivision * 0 + 1] = { upperPosition, Vector3::up };
        vertices[i + kSubdivision * 1 + 1] = { upperPosition, sideNormal };
        vertices[i + kSubdivision * 2 + 1] = { lowerPosition, sideNormal };
        vertices[i + kSubdivision * 3 + 1] = { lowerPosition, Vector3::down };
    }
    vertices.back() = { { 0.0f,-kHalfHeight, 0.0f }, Vector3::down };

    size_t upperFaceIndex = 0;
    size_t sideFaceIndex = kSubdivision * 3;
    size_t lowerFaceIndex = kSubdivision * 3 * 3;

    uint16_t round = uint16_t(kSubdivision);
    for (uint16_t i = 0; i < kSubdivision; ++i) {
        uint16_t j = (i + 1) % round;


        uint16_t top = 0;
        uint16_t upperFace0 = i + 1;
        uint16_t upperFace1 = j + 1;

        indices[upperFaceIndex++] = top;
        indices[upperFaceIndex++] = upperFace1;
        indices[upperFaceIndex++] = upperFace0;

        uint16_t sideFaceBase = round + 1;
        uint16_t sideFaceUpper0 = i + sideFaceBase;
        uint16_t sideFaceUpper1 = j + sideFaceBase;
        uint16_t sideFaceLower0 = i + round + sideFaceBase;
        uint16_t sideFaceLower1 = j + round + sideFaceBase;

        indices[sideFaceIndex++] = sideFaceUpper0;
        indices[sideFaceIndex++] = sideFaceUpper1;
        indices[sideFaceIndex++] = sideFaceLower0;

        indices[sideFaceIndex++] = sideFaceLower0;
        indices[sideFaceIndex++] = sideFaceUpper1;
        indices[sideFaceIndex++] = sideFaceLower1;

        uint16_t lowerFace0 = i + 1 + round * 3;
        uint16_t lowerFace1 = j + 1 + round * 3;
        uint16_t bottom = uint16_t(kVertexCount) - 1;

        indices[lowerFaceIndex++] = lowerFace0;
        indices[lowerFaceIndex++] = lowerFace1;
        indices[lowerFaceIndex++] = bottom;
    }

    cylinder_ = RegisterMesh(vertices, indices);
}

void Renderer::CreateMeshCone() {
    const size_t kSubdivision = 16;
    const float kAngleEvery = Math::TwoPi / float(kSubdivision);
    // 頂点 + 屋根外周分割数 + 底面分割数 + 底面中央
    // 天板はフラット側面はスムーズ
    const size_t kVertexCount = 1 + kSubdivision + kSubdivision + 1;
    const size_t kIndexCount = kSubdivision * 2 * 3;
    const float kHeight = 1.0f;
    const float kHalfHeight = kHeight * 0.5f;

    std::vector<Vertex> vertices(kVertexCount);
    std::vector<uint16_t> indices(kIndexCount);

    // verticesレイアウト
    // 頂点 1, 屋根外周 分割数, 底面分割数　分割数, 底面中央 1　

    vertices[0] = { { 0.0f,kHalfHeight, 0.0f }, Vector3::up };
    for (size_t i = 0; i < kSubdivision; ++i) {
        float angle = kAngleEvery * i;
        float c = std::cos(angle), s = std::sin(angle);
        Vector3 position = { c, -kHalfHeight, s };
        Vector3 normal = { c, 0.0f, s };
        vertices[i + kSubdivision * 0 + 1] = { position, normal.Normalized() };
        vertices[i + kSubdivision * 1 + 1] = { position, Vector3::down };
    }
    vertices.back() = { { 0.0f,-kHalfHeight, 0.0f }, Vector3::down };

    size_t roofFaceindex = 0;
    size_t bottomFaceindex = kSubdivision * 3;

    uint16_t round = uint16_t(kSubdivision);
    for (uint16_t i = 0; i < kSubdivision; ++i) {
        uint16_t j = (i + 1) % round;

        uint16_t apex = 0;
        uint16_t roofFaceIndex0 = i + 1;
        uint16_t roofFaceIndex1 = j + 1;

        indices[roofFaceindex++] = apex;
        indices[roofFaceindex++] = roofFaceIndex1;
        indices[roofFaceindex++] = roofFaceIndex0;

        uint16_t bottomFaceIndex0 = i + 1 + round;
        uint16_t bottomFaceIndex1 = j + 1 + round;
        uint16_t bottom = uint16_t(kVertexCount) - 1;

        indices[bottomFaceindex++] = bottomFaceIndex0;
        indices[bottomFaceindex++] = bottomFaceIndex1;
        indices[bottomFaceindex++] = bottom;
    }

    cone_ = RegisterMesh(vertices, indices);
}