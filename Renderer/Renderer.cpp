#include "Renderer.hpp"

#include <Windows.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <wrl.h>

#include <cassert>
#include <cstdint>
#include <format>
#include <unordered_map>

#include "Externals/ImGui/imgui.h"
#include "Externals/ImGui/imgui_impl_dx12.h"
#include "Externals/ImGui/imgui_impl_win32.h"

#include "ShaderUtils.hpp"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")

//#include "Input.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {
    template<class T>

    using ComPtr = Microsoft::WRL::ComPtr<T>;

    struct Vertex {
        Vector3 position;
        Vector3 normal;
    };
    struct Mesh {
        ComPtr<ID3D12Resource> vertexBuffer;
        ComPtr<ID3D12Resource> indexBuffer;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW indexBufferView{};
        std::uint32_t indexCount{};
    };
    struct Instance {
        Vector4 mat0;
        Vector4 mat1;
        Vector4 mat2;
        Vector4 color;
    };
    struct DynamicBuffer {
        std::size_t size{};
        ComPtr<ID3D12Resource> resource;
        void* mappedPtr{ nullptr };

        template<class T>
        T* GetMappedPtr() { return reinterpret_cast<T*>(mappedPtr); }
    };

    struct SceneConstant {
        Matrix4x4 viewProjectionMatrix;
        Vector4 lightColor;
        Vector3 lightDirection;
        float lightIntensity;
    };

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
        std::vector<Vertex>* smooth_vertices) {
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

class Renderer::Impl {
public:
    void Initialize(const std::wstring& window_title, const std::uint32_t window_width, const std::uint32_t window_height) {
        // Windows関連
        {
            // ウィンドウクラスを生成
            WNDCLASS wc{};
            wc.lpfnWndProc = WindowProc;	// ウィンドウプロシージャ
            wc.lpszClassName = L"CG2WindowClass";	// ウィンドウクラス名
            wc.hInstance = GetModuleHandle(nullptr);	// インスタンスハンドル
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);	// カーソル
            RegisterClass(&wc);	// ウィンドウクラスを登録

            // ウィンドウサイズを表す構造体にクライアント領域を入れる
            RECT wrc{ 0,0,static_cast<LONG>(window_width),static_cast<LONG>(window_height) };
            // クライアント領域を元に実際のサイズにwrcを変更してもらう
            AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

            // ウィンドウの生成
            hwnd_ = CreateWindow(
                wc.lpszClassName,		// 利用するクラス名
                window_title.c_str(),				// タイトルバーの文字
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

        //Input::Initialize(hwnd_);
        // ファクトリとデバイス
        {
#ifdef _DEBUG	
            // デバッグ時のみ
            ComPtr<ID3D12Debug1> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
                // デバッグレイヤーを有効化する
                debugController->EnableDebugLayer();
                // さらにGPU側でもチェックを行えるようにする
                debugController->SetEnableGPUBasedValidation(TRUE);
            }
#endif
            HRESULT hr = S_FALSE;

            // DXGIファクトリーの生成
            hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
            assert(SUCCEEDED(hr));

            // 使用するアダプタ用の変数
            ComPtr<IDXGIAdapter4> useAdapter;

            // 良い順にアダプターを頼む
            for (uint32_t i = 0;
                dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND;
                ++i) {
                // アダプター情報を取得
                DXGI_ADAPTER_DESC3 adapterDesc{};
                hr = useAdapter->GetDesc3(&adapterDesc);
                assert(SUCCEEDED(hr));
                // ソフトウェアアダプタでなければ採用
                if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
                    // 採用したアダプタ情報を出力
                    OutputDebugStringW(std::format(L"Use Adapter:{}\n", adapterDesc.Description).c_str());
                    break;
                }
                useAdapter = nullptr; // ソフトウェアアダプタは見なかったことにする
            }
            assert(useAdapter != nullptr);


            // 機能レベルとログ出力用の文字列
            D3D_FEATURE_LEVEL featureLevels[] = {
                D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0
            };
            const char* featureLevelStrings[] = { "12.2", "12.1", "12.0" };
            // 高い順に生成できるか試していく
            for (size_t i = 0; i < _countof(featureLevels); ++i) {
                // 採用したアダプターデバイスを生成
                hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
                // 指定した機能レベルでデバイスが生成できたかを確認
                if (SUCCEEDED(hr)) {
                    // 生成できたのでログ出力を行ってループを抜ける
                    OutputDebugStringA(std::format("FeatureLevel : {}\n", featureLevelStrings[i]).c_str());
                    break;
                }
            }
            assert(device_ != nullptr);
            OutputDebugStringA("Complete create D3D12Device!!!\n"); // 初期化完了のログを出す

#ifdef _DEBUG
            // デバッグ時のみ
            ComPtr<ID3D12InfoQueue> infoQueue;
            if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
                // やばいエラーの時に止まる
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                // エラーの時に止まる
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                // 警告時に止まる
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
                // 抑制するメッセージのID
                D3D12_MESSAGE_ID denyIds[] = {
                    D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
                };
                // 抑制するレベル
                D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
                D3D12_INFO_QUEUE_FILTER filter{};
                filter.DenyList.NumIDs = _countof(denyIds);
                filter.DenyList.pIDList = denyIds;
                filter.DenyList.NumSeverities = _countof(severities);
                filter.DenyList.pSeverityList = severities;
                // 指定したメッセージの表示を抑制する
                infoQueue->PushStorageFilter(&filter);
            }
#endif
        }
        // コマンド関連
        {
            HRESULT hr = S_FALSE;
            // コマンドキューを生成
            D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
            hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
            assert(SUCCEEDED(hr));

            for (uint32_t i = 0; i < kSwapChainBufferCount; ++i) {
                // コマンドアロケータを生成
                hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators_[i]));
                assert(SUCCEEDED(hr));
            }

            // コマンドリストを生成
            hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators_[0].Get(), nullptr, IID_PPV_ARGS(&commandList_));
            assert(SUCCEEDED(hr));
            hr = commandList_->Close();
            assert(SUCCEEDED(hr));

            // フェンスを生成
            hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
            assert(SUCCEEDED(hr));

            fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
            assert(fenceEvent_ != nullptr);
        }
        // ディスクリプタヒープ
        {
            rtvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kSwapChainBufferCount, false);
            dsvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
            srvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true);
        }
        // スワップチェーン関連
        {
            WINDOWINFO winInfo{};
            winInfo.cbSize = sizeof(winInfo);
            if (!GetWindowInfo(hwnd_, &winInfo)) {
                assert(false);
            }

            HRESULT hr = S_FALSE;
            // スワップチェーンの設定
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
            swapChainDesc.Width = static_cast<uint32_t>(winInfo.rcClient.right - winInfo.rcClient.left);		// 画面幅
            swapChainDesc.Height = static_cast<uint32_t>(winInfo.rcClient.bottom - winInfo.rcClient.top);	// 画面高
            swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// 色の形式
            swapChainDesc.SampleDesc.Count = 1;					// マルチサンプル市内
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// 描画ターゲットとして利用する
            swapChainDesc.BufferCount = kSwapChainBufferCount;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// モニタに移したら、中身を破棄
            // スワップチェーンを生成
            ComPtr<IDXGISwapChain1> tmpSwapChain;
            hr = dxgiFactory_->CreateSwapChainForHwnd(commandQueue_.Get(), hwnd_, &swapChainDesc, nullptr, nullptr, &tmpSwapChain);
            assert(SUCCEEDED(hr));
            hr = tmpSwapChain.As(&swapChain_);
            assert(SUCCEEDED(hr));

            // SwapChainResourceの生成とRTVの生成
            for (uint32_t i = 0; i < kSwapChainBufferCount; ++i) {
                // SwapChainからResourceを引っ張ってくる
                hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
                assert(SUCCEEDED(hr));
                // RTVの設定
                D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
                rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                // ディスクリプタの先頭を取得
                rtvHandles_[i] = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
                // ディスクリプタハンドルをずらす
                rtvHandles_[i].ptr += static_cast<size_t>(i) * device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                // RTVを生成
                device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc, rtvHandles_[i]);

                {
                    D3D12_RESOURCE_DESC depthStencilResourceDesc{};
                    depthStencilResourceDesc.Width = swapChainDesc.Width;
                    depthStencilResourceDesc.Height = swapChainDesc.Height;
                    depthStencilResourceDesc.MipLevels = 1;
                    depthStencilResourceDesc.DepthOrArraySize = 1;
                    depthStencilResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    depthStencilResourceDesc.SampleDesc.Count = 1;
                    depthStencilResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                    depthStencilResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

                    D3D12_HEAP_PROPERTIES heapProperties{};
                    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

                    D3D12_CLEAR_VALUE depthClearValue{};
                    depthClearValue.DepthStencil.Depth = 1.0f;
                    depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

                    hr = device_->CreateCommittedResource(
                        &heapProperties,
                        D3D12_HEAP_FLAG_NONE,
                        &depthStencilResourceDesc,
                        D3D12_RESOURCE_STATE_DEPTH_WRITE,
                        &depthClearValue,
                        IID_PPV_ARGS(&depthStencilResource_));
                    assert(SUCCEEDED(hr));

                    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
                    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                    dsvHandle_ = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
                    device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvHandle_);
                }
                viewport_.Width = static_cast<float>(swapChainDesc.Width);
                viewport_.Height = static_cast<float>(swapChainDesc.Height);
                viewport_.TopLeftX = 0.0f;
                viewport_.TopLeftY = 0.0f;
                viewport_.MinDepth = 0.0f;
                viewport_.MaxDepth = 1.0f;

                scissorRect_.left = 0;
                scissorRect_.right = static_cast<LONG>(swapChainDesc.Width);
                scissorRect_.top = 0;
                scissorRect_.bottom = static_cast<LONG>(swapChainDesc.Height);
            }
        }
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui::StyleColorsDark();
            ImGui_ImplWin32_Init(hwnd_);
            ImGui_ImplDX12_Init(
                device_.Get(),
                kSwapChainBufferCount,
                DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                srvDescriptorHeap_.Get(),
                srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
                srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart());
        }
        // ルートシグネチャ、パイプライン
        {

            HRESULT hr = S_FALSE;

            // カメラ用
            D3D12_ROOT_PARAMETER rootParameters[1] = {};
            rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            rootParameters[0].Descriptor.ShaderRegister = 0;

            D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
            descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            descriptionRootSignature.pParameters = rootParameters;
            descriptionRootSignature.NumParameters = _countof(rootParameters);

            ComPtr<ID3DBlob> signatureBlob = nullptr;
            ComPtr<ID3DBlob> errorBlob = nullptr;

            hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
            if (FAILED(hr)) {
                OutputDebugStringA(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
                assert(false);
            }
            hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
            assert(SUCCEEDED(hr));

            // インプットレイアウト
            D3D12_INPUT_ELEMENT_DESC inputElementDescs[6] = {};

            // 頂点
            inputElementDescs[0].SemanticName = "POSITION";
            inputElementDescs[0].SemanticIndex = 0;
            inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
            inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            inputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

            inputElementDescs[1].SemanticName = "NORMAL";
            inputElementDescs[1].SemanticIndex = 0;
            inputElementDescs[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
            inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            inputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

            // インスタンス
            inputElementDescs[2].SemanticName = "MATRIX";
            inputElementDescs[2].SemanticIndex = 0;
            inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            inputElementDescs[2].InputSlot = 1;
            inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            inputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
            inputElementDescs[2].InstanceDataStepRate = 1;

            inputElementDescs[3].SemanticName = "MATRIX";
            inputElementDescs[3].SemanticIndex = 1;
            inputElementDescs[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            inputElementDescs[3].InputSlot = 1;
            inputElementDescs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            inputElementDescs[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
            inputElementDescs[3].InstanceDataStepRate = 1;

            inputElementDescs[4].SemanticName = "MATRIX";
            inputElementDescs[4].SemanticIndex = 2;
            inputElementDescs[4].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            inputElementDescs[4].InputSlot = 1;
            inputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            inputElementDescs[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
            inputElementDescs[4].InstanceDataStepRate = 1;

            inputElementDescs[5].SemanticName = "COLOR";
            inputElementDescs[5].SemanticIndex = 0;
            inputElementDescs[5].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            inputElementDescs[5].InputSlot = 1;
            inputElementDescs[5].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            inputElementDescs[5].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
            inputElementDescs[5].InstanceDataStepRate = 1;

            D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
            inputLayoutDesc.pInputElementDescs = inputElementDescs;
            inputLayoutDesc.NumElements = _countof(inputElementDescs);

            // ブレンドステート
            D3D12_BLEND_DESC blendDesc{};
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            blendDesc.RenderTarget[0].BlendEnable = true;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

            D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
            depthStencilDesc.DepthEnable = true;
            depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

            // ラスタライザステート
            D3D12_RASTERIZER_DESC rasterizerDesc{};
            rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
            rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

            ShaderManager shader_manager;
            shader_manager.Initialize();
#ifdef _DEBUG
            // シェーダーをコンパイル
            auto vertexShader = shader_manager.CompileShader(L"object_vs.hlsl", L"vs_6_0");
            assert(vertexShader);
            auto pixelShader = shader_manager.CompileShader(L"object_ps.hlsl", L"ps_6_0");
            assert(pixelShader);
#else
            // シェーダーをコンパイル
            auto vertexShader = shader_manager.LoadShader(L"object_vs.cso");
            if (!vertexShader) {
                vertexShader = shader_manager.CompileShader(L"object_vs.hlsl", L"vs_6_0", false);
            }
            auto pixelShader = shader_manager.LoadShader(L"object_ps.cso");
            if (!pixelShader) {
                pixelShader = shader_manager.CompileShader(L"object_ps.hlsl", L"ps_6_0", false);
            }
#endif // !_DEBUG


            // パイプライン生成
            D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
            graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();
            graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
            graphicsPipelineStateDesc.VS = *vertexShader;
            graphicsPipelineStateDesc.PS = *pixelShader;
            graphicsPipelineStateDesc.BlendState = blendDesc;
            graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
            graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
            graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
            graphicsPipelineStateDesc.NumRenderTargets = 1;
            graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            graphicsPipelineStateDesc.SampleDesc.Count = 1;
            graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

            hr = device_->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&objPipelineState_));
            assert(SUCCEEDED(hr));

#ifdef _DEBUG
            // シェーダーをコンパイル
            vertexShader = shader_manager.CompileShader(L"wire_frame_vs.hlsl", L"vs_6_0");
            assert(vertexShader);
            pixelShader = shader_manager.CompileShader(L"wire_frame_ps.hlsl", L"ps_6_0");
            assert(pixelShader);
#else
            // シェーダーをコンパイル
            vertexShader = shader_manager.LoadShader(L"wire_frame_vs.cso");
            if (!vertexShader) {
                vertexShader = shader_manager.CompileShader(L"wire_frame_vs.hlsl", L"vs_6_0", false);
            }
            pixelShader = shader_manager.LoadShader(L"wire_frame_ps.cso");
            if (!pixelShader) {
                pixelShader = shader_manager.CompileShader(L"wire_frame_ps.hlsl", L"ps_6_0", false);
            }
#endif // !_DEBUG

            rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
            rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
            graphicsPipelineStateDesc.VS = *vertexShader;
            graphicsPipelineStateDesc.PS = *pixelShader;
            graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
            hr = device_->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&wireFramePipelineState));
        }
        // インスタンシング用バッファ
        {
            for (uint32_t i = 0; i < kSwapChainBufferCount; ++i) {
                std::size_t strideSize = sizeof(Instance);
                instancingBuffers_[i].size = strideSize * kMaxObjectCount;
                instancingBuffers_[i].resource = CreateBufferResource(instancingBuffers_[i].size);
                instancingBuffers_[i].resource->Map(0, nullptr, &instancingBuffers_[i].mappedPtr);
                instancingBufferViews_[i].BufferLocation = instancingBuffers_[i].resource->GetGPUVirtualAddress();
                instancingBufferViews_[i].StrideInBytes = static_cast<std::uint32_t>(strideSize);
                instancingBufferViews_[i].SizeInBytes = static_cast<std::uint32_t>(instancingBuffers_[i].size);

                sceneBuffers_[i].size = sizeof(SceneConstant);
                sceneBuffers_[i].resource = CreateBufferResource(sizeof(SceneConstant));
                sceneBuffers_[i].resource->Map(0, nullptr, reinterpret_cast<void**>(&sceneBuffers_[i].mappedPtr));
                Vector3 translate = { 0.0f,1.0f,-6.0f };
                Matrix4x4 view = Matrix4x4::MakeAffineInverse(Matrix4x4::MakeLookRotation(-translate), translate);
                projectionMatrix_ = Matrix4x4::MakePerspectiveProjection(45.0f * Math::ToRadian, static_cast<float>(window_width) / static_cast<float>(window_height), 0.1f, 100.0f);
                SceneConstant scene{};
                scene.viewProjectionMatrix = view * projectionMatrix_;
                scene.lightColor = { 1.0f,1.0f,1.0f,1.0f };
                scene.lightDirection = { -1.0f,-1.0f,1.0f };
                scene.lightIntensity = 1.0f;
                auto ptr = sceneBuffers_[i].GetMappedPtr<SceneConstant>();
                *ptr = scene;
            }

        }

        ShowWindow(hwnd_, SW_SHOW);
    }
    void StartRendering() {
        // これから書き込むバックバッファインデックスを取得
        uint32_t backBufferIndex = swapChain_->GetCurrentBackBufferIndex();
        HRESULT hr = S_FALSE;
        // 次フレーム用のコマンドリストを準備
        hr = commandAllocators_[backBufferIndex]->Reset();
        assert(SUCCEEDED(hr));
        hr = commandList_->Reset(commandAllocators_[backBufferIndex].Get(), nullptr);
        assert(SUCCEEDED(hr));

        ImGui_ImplWin32_NewFrame();
        ImGui_ImplDX12_NewFrame();
        ImGui::NewFrame();

        // レンダ―ターゲットに遷移
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        // TransitionBarrierを張る
        commandList_->ResourceBarrier(1, &barrier);
        // 描画先のRTVを設定
        commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, &dsvHandle_);
        // 指定した色で画面全体をクリア
        float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
        commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);
        commandList_->ClearDepthStencilView(dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        commandList_->RSSetViewports(1, &viewport_);
        commandList_->RSSetScissorRects(1, &scissorRect_);

        ID3D12DescriptorHeap* ppHeaps[] = { srvDescriptorHeap_.Get() };
        commandList_->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    }
    void EndRendering() {

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList_.Get());

        uint32_t backBufferIndex = swapChain_->GetCurrentBackBufferIndex();
        // RenderTargetからPresentへ
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        // TransitionBarrierを張る
        commandList_->ResourceBarrier(1, &barrier);

        HRESULT hr = S_FALSE;
        // コマンドリストの内容を確定
        hr = commandList_->Close();
        assert(SUCCEEDED(hr));
        // Fenceの値が指定したSignal値にたどり着いているか確認する
        // GetCompletedValueの初期値はFence作成時に渡した初期値
        if (fence_->GetCompletedValue() < fenceValue_) {
            // 指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
            fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
            // イベントを待つ
            WaitForSingleObject(fenceEvent_, INFINITE);
        }
        // GPUにコマンドリストの実行を行わせる
        ID3D12CommandList* commandLists[] = { commandList_.Get() };
        commandQueue_->ExecuteCommandLists(1, commandLists);
        // GPUとOSに画面交換を行うよう通知する
        swapChain_->Present(1, 0);
        // GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
        hr = commandQueue_->Signal(fence_.Get(), ++fenceValue_);
        assert(SUCCEEDED(hr));
    }
    void Finalize() {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CloseHandle(fenceEvent_);
        CloseWindow(hwnd_);
    }

    std::size_t RegisterMesh(const std::vector<Vertex>& vertices, const std::vector<std::uint16_t> indices) {
        std::size_t handle = meshes_.size();
        auto& mesh = meshes_.emplace_back();
        std::size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
        std::size_t indexBufferSize = indices.size() * sizeof(std::uint16_t);
        mesh.vertexBuffer = CreateBufferResource(vertexBufferSize);
        mesh.indexBuffer = CreateBufferResource(indexBufferSize);

        void* mappedPtr = nullptr;
        mesh.vertexBuffer->Map(0, nullptr, &mappedPtr);
        memcpy(mappedPtr, vertices.data(), vertexBufferSize);

        mesh.indexBuffer->Map(0, nullptr, &mappedPtr);
        memcpy(mappedPtr, indices.data(), indexBufferSize);

        mesh.vertexBufferView.BufferLocation = mesh.vertexBuffer->GetGPUVirtualAddress();
        mesh.vertexBufferView.SizeInBytes = static_cast<std::uint32_t>(vertexBufferSize);
        mesh.vertexBufferView.StrideInBytes = static_cast<std::uint32_t>(sizeof(Vertex));
        mesh.indexBufferView.BufferLocation = mesh.indexBuffer->GetGPUVirtualAddress();
        mesh.indexBufferView.SizeInBytes = static_cast<std::uint32_t>(indexBufferSize);
        mesh.indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        mesh.indexCount = static_cast<std::uint32_t>(indices.size());
        return handle;
    }

    void AddWireFrameInstance(std::size_t mesh_handle, const Matrix4x4& world_matrix, const Vector4& color) {
        assert(objectCounter_ < kMaxObjectCount);
        Matrix4x4 mat = world_matrix.Transpose();
        Instance instance{};
        instance.mat0 = mat.GetRow(0);
        instance.mat1 = mat.GetRow(1);
        instance.mat2 = mat.GetRow(2);
        instance.color = color;
        wireFrameInstancesMap_[mesh_handle].emplace_back(instance);
        ++objectCounter_;
    }
    void AddObjInstance(std::size_t mesh_handle, const Matrix4x4& world_matrix, const Vector4& color) {
        assert(objectCounter_ < kMaxObjectCount);
        Matrix4x4 mat = world_matrix.Transpose();
        Instance instance{};
        instance.mat0 = mat.GetRow(0);
        instance.mat1 = mat.GetRow(1);
        instance.mat2 = mat.GetRow(2);
        instance.color = color;
        objInstancesMap_[mesh_handle].emplace_back(instance);
        ++objectCounter_;
    }

    void DrawObjects() {
        uint32_t backBufferIndex = swapChain_->GetCurrentBackBufferIndex();
        Instance* copyDest = instancingBuffers_[backBufferIndex].GetMappedPtr<Instance>();
        uint32_t instanceLocation = 0;

        commandList_->SetGraphicsRootSignature(rootSignature_.Get());
        commandList_->SetGraphicsRootConstantBufferView(0, sceneBuffers_[backBufferIndex].resource->GetGPUVirtualAddress());
        commandList_->SetPipelineState(wireFramePipelineState.Get());
        commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


        for (auto& itr : wireFrameInstancesMap_) {
            std::size_t meshIndex = itr.first;
            auto& instances = itr.second;
            std::size_t copySize = instances.size() * sizeof(instances[0]);
            memcpy(copyDest, instances.data(), copySize);
            copyDest += instances.size();
            D3D12_VERTEX_BUFFER_VIEW views[] = {
                meshes_[meshIndex].vertexBufferView,
                instancingBufferViews_[backBufferIndex]
            };
            commandList_->IASetVertexBuffers(0, _countof(views), views);
            commandList_->IASetIndexBuffer(&meshes_[meshIndex].indexBufferView);
            commandList_->DrawIndexedInstanced(
                meshes_[meshIndex].indexCount,
                static_cast<std::uint32_t>(instances.size()),
                0, 0,
                instanceLocation);
            instanceLocation += static_cast<std::uint32_t>(instances.size());
        }

        commandList_->SetPipelineState(objPipelineState_.Get());
        commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for (auto& itr : objInstancesMap_) {
            std::size_t meshIndex = itr.first;
            auto& instances = itr.second;
            std::size_t copySize = instances.size() * sizeof(instances[0]);
            memcpy(copyDest, instances.data(), copySize);
            copyDest += instances.size();
            D3D12_VERTEX_BUFFER_VIEW views[] = {
                meshes_[meshIndex].vertexBufferView,
                instancingBufferViews_[backBufferIndex]
            };
            commandList_->IASetVertexBuffers(0, _countof(views), views);
            commandList_->IASetIndexBuffer(&meshes_[meshIndex].indexBufferView);
            commandList_->DrawIndexedInstanced(
                meshes_[meshIndex].indexCount,
                static_cast<std::uint32_t>(instances.size()),
                0, 0,
                instanceLocation);
            instanceLocation += static_cast<std::uint32_t>(instances.size());
        }

        assert(objectCounter_ == instanceLocation);

        wireFrameInstancesMap_.clear();
        objInstancesMap_.clear();
        objectCounter_ = 0;
    }

    void UpdateCamera(const Vector3& cameraPos, const Vector3& cameraRot) {
        auto scene = sceneBuffers_[swapChain_->GetCurrentBackBufferIndex()].GetMappedPtr<SceneConstant>();

        Matrix4x4 view = Matrix4x4::MakeAffineTransform(Vector3::one, cameraRot, cameraPos).Inverse();
        scene->viewProjectionMatrix = view * projectionMatrix_;
    }

    void UpdateLight(const Vector3& dir, const Vector4& color, float intensity) {
        auto scene = sceneBuffers_[swapChain_->GetCurrentBackBufferIndex()].GetMappedPtr<SceneConstant>();
        scene->lightDirection = dir;
        scene->lightColor = color;
        scene->lightIntensity = intensity;
    }

private:
    ComPtr<ID3D12Resource> CreateBufferResource(size_t size_in_bytes) {
        // アップロードヒープ
        D3D12_HEAP_PROPERTIES uploadHeapProperties{};
        uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        // 頂点バッファの設定
        D3D12_RESOURCE_DESC bufferDesc{};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = size_in_bytes;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> result;
        // 頂点バッファを生成
        HRESULT hr = S_FALSE;
        hr = device_->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&result));
        assert(SUCCEEDED(hr));
        return result;
    }
    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32_t descriptor_count, bool shader_visible) {
        D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
        descriptorHeapDesc.Type = heap_type;
        descriptorHeapDesc.NumDescriptors = descriptor_count;
        descriptorHeapDesc.Flags = shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;
        HRESULT hr = S_FALSE;
        hr = device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
        assert(SUCCEEDED(hr));
        return descriptorHeap;
    }
    static const uint32_t kSwapChainBufferCount = 2;

    HWND hwnd_{ nullptr };
    ComPtr<IDXGIFactory7> dxgiFactory_;
    ComPtr<ID3D12Device> device_;

    ComPtr<ID3D12CommandQueue> commandQueue_;
    ComPtr<ID3D12CommandAllocator> commandAllocators_[kSwapChainBufferCount];
    ComPtr<ID3D12GraphicsCommandList> commandList_;
    ComPtr<ID3D12Fence> fence_;
    uint64_t fenceValue_{ 0 };
    HANDLE fenceEvent_{ nullptr };

    ComPtr<IDXGISwapChain4> swapChain_;
    ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
    ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
    ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
    ComPtr<ID3D12Resource> swapChainResources_[kSwapChainBufferCount];
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[kSwapChainBufferCount]{};
    ComPtr<ID3D12Resource> depthStencilResource_;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_{};
    D3D12_VIEWPORT viewport_{};
    D3D12_RECT scissorRect_{};

    ComPtr<ID3D12RootSignature> rootSignature_;
    ComPtr<ID3D12PipelineState> wireFramePipelineState;
    ComPtr<ID3D12PipelineState> objPipelineState_;

    std::vector<Mesh> meshes_;
    DynamicBuffer instancingBuffers_[kSwapChainBufferCount];
    D3D12_VERTEX_BUFFER_VIEW instancingBufferViews_[kSwapChainBufferCount]{};
    std::unordered_map<std::size_t, std::vector<Instance>> objInstancesMap_;
    std::unordered_map<std::size_t, std::vector<Instance>> wireFrameInstancesMap_;
    std::size_t objectCounter_{ 0 };

    DynamicBuffer sceneBuffers_[kSwapChainBufferCount];
    Matrix4x4 projectionMatrix_;
};

Renderer::Renderer() : pimpl_(new Impl) {
}

Renderer::~Renderer() {
    delete pimpl_;
}

void Renderer::Initailize(const std::wstring& window_title, const std::uint32_t window_width, const std::uint32_t window_height) {
    pimpl_->Initialize(window_title, window_width, window_height);
    {
        std::vector<Vertex> vertices = {
            {{ -0.5f, 0.0f, -0.5f }, {0.0f,1.0f,0.0f}},
            {{ -0.5f, 0.0f,  0.5f }, {0.0f,1.0f,0.0f}},
            {{  0.5f, 0.0f, -0.5f }, {0.0f,1.0f,0.0f}},
            {{  0.5f, 0.0f,  0.5f }, {0.0f,1.0f,0.0f}} };
        std::vector<uint16_t> indices = {
            0,1,2,
            1,3,2 };
        plane_ = pimpl_->RegisterMesh(vertices, indices);
    }
    {
        const int kSurfaceCount = 6;
        const int kVertexCount = 24;
        const int kIndexCount = 36;
        std::vector<Vertex> vertices(kVertexCount);
        std::vector<uint16_t> indices(kIndexCount);
        {
            enum {
                LTN,    // 左上前
                LBN,    // 左下前
                RTN,    // 右上前
                RBN,    // 右下前
                LTF,    // 左上奥
                LBF,    // 左下奥
                RTF,    // 右上奥
                RBF,    // 右下奥
            };

            Vector3 position[8] = {
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
        }

        // 法線
        {
            Vector3 normal[6] = {
                -Vector3::unitZ, // 前面
                 Vector3::unitZ, // 後面
                 Vector3::unitX, // 右面
                -Vector3::unitX, // 左面
                 Vector3::unitY, // 上面
                -Vector3::unitY, // 下面
            };

            for (size_t i = 0; i < kSurfaceCount; i++) {
                size_t j = i * 4;
                vertices[j + 0].normal = normal[i];
                vertices[j + 1].normal = normal[i];
                vertices[j + 2].normal = normal[i];
                vertices[j + 3].normal = normal[i];
            }
        }
        {
            uint16_t i = 0;
            for (uint16_t j = 0; j < kSurfaceCount; j++) {
                uint16_t k = j * 4;

                indices[i++] = k + 1;
                indices[i++] = k;
                indices[i++] = k + 2;
                indices[i++] = k + 1;
                indices[i++] = k + 2;
                indices[i++] = k + 3;
            }
        }
        box_ = pimpl_->RegisterMesh(vertices, indices);
    }

    {
        // 球を作成
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
        {
            const int32_t kSubdivision = 16;

            const float kLonEvery = Math::TwoPi / float(kSubdivision);
            const float kLatEvery = Math::Pi / float(kSubdivision);
            const size_t kLatVertexCount = size_t(kSubdivision + 1);
            const size_t kLonVertexCount = size_t(kSubdivision + 1);
            const size_t kVertexCount = kLonVertexCount * kLatVertexCount;

            vertices.resize(kVertexCount);

            auto CalcPosition = [](float lat, float lon) {
                return Vector3{
                    { std::cos(lat) * std::cos(lon) },
                    { std::sin(lat) },
                    { std::cos(lat) * std::sin(lon) }
                };
            };



            for (size_t latIndex = 0; latIndex < kLonVertexCount; ++latIndex) {
                float lat = -Math::HalfPi + kLatEvery * latIndex;

                for (size_t lonIndex = 0; lonIndex < kLatVertexCount; ++lonIndex) {
                    float lon = lonIndex * kLonEvery;

                    size_t vertexIndex = latIndex * kLatVertexCount + lonIndex;
                    vertices[vertexIndex].position = CalcPosition(lat, lon);
                    vertices[vertexIndex].normal = static_cast<Vector3>(vertices[vertexIndex].position);
                }
            }

            const size_t kIndexCount = size_t(kSubdivision * kSubdivision) * 6;
            indices.resize(kIndexCount);

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
        }
        sphere_ = pimpl_->RegisterMesh(vertices, indices);
    }



}

void Renderer::StartRendering() {
    pimpl_->StartRendering();
}

void Renderer::EndRendering() {
    pimpl_->DrawObjects();
    pimpl_->EndRendering();
}

void Renderer::Finalize() {
    pimpl_->Finalize();
}

std::size_t Renderer::RegisterMesh(const std::vector<Vector3>& positions, const std::vector<std::uint16_t> indices) {
    std::vector<Vertex> vertices;
    CalcNormals(positions, indices, &vertices);
    return pimpl_->RegisterMesh(vertices, indices);
}

void Renderer::DrawPlane(const Matrix4x4& world_matrix, const Vector4& color, DrawMode draw_mode) {
    if (draw_mode == DrawMode::kObject) {
        pimpl_->AddObjInstance(plane_, world_matrix, color);
        return;
    }
    pimpl_->AddWireFrameInstance(plane_, world_matrix, color);
}

void Renderer::DrawBox(const Matrix4x4& world_matrix, const Vector4& color, DrawMode draw_mode) {
    if (draw_mode == DrawMode::kObject) {
        pimpl_->AddObjInstance(box_, world_matrix, color);
        return;
    }
    pimpl_->AddWireFrameInstance(box_, world_matrix, color);
}

void Renderer::DrawSphere(const Matrix4x4& world_matrix, const Vector4& color, DrawMode draw_mode) {
    if (draw_mode == DrawMode::kObject) {
        pimpl_->AddObjInstance(sphere_, world_matrix, color);
        return;
    }
    pimpl_->AddWireFrameInstance(sphere_, world_matrix, color);
}

void Renderer::DrawObject(std::size_t mesh_handle, const Vector3& scale, const Quaternion& rotate, const Vector3& translate, const Vector4& color) {
    pimpl_->AddObjInstance(mesh_handle, Matrix4x4::MakeAffineTransform(scale, rotate, translate), color);
    // pimpl_->AddLineInstance(mesh_handle, Matrix4x4::MakeAffineTransform(scale, rotate, translate), color);
}

void Renderer::SetCamera(const Vector3& pos, const Vector3& rot) {
    pimpl_->UpdateCamera(pos, rot);
}

void Renderer::SetLight(const Vector3& direction, const Vector4& color, float intensity) {
    pimpl_->UpdateLight(direction, color, intensity);
}
