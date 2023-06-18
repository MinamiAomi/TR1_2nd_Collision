#include "ShaderUtils.hpp"

#include <cassert>
#include <format>
#include <fstream>

#pragma comment(lib,"dxcompiler.lib")

namespace {
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;
};

std::shared_ptr<Shader> ShaderManager::CompileShader(const std::filesystem::path& file_name, const std::wstring& profile, bool is_output_compiled_file) {
    assert(file_name.extension() == ".hlsl");

    auto full_path = directory_ / file_name;
    OutputDebugStringW(std::format(L"Begin CompileShader, path:{}, profile:{}\n", full_path.wstring(), profile).c_str());

    ComPtr<IDxcBlobEncoding> shader_source;
    if (FAILED(dxc_utils_->LoadFile(full_path.c_str(), nullptr, &shader_source))) {
        OutputDebugStringA("Failed load shader file\n");
        assert(false);
        return {};
    }

    DxcBuffer shader_source_buffer{};
    shader_source_buffer.Ptr = shader_source->GetBufferPointer();
    shader_source_buffer.Size = shader_source->GetBufferSize();
    shader_source_buffer.Encoding = DXC_CP_UTF8;

    LPCWSTR arguments[] = {
        file_name.c_str(),
        L"-E", L"main",
        L"-T", profile.c_str(),
        L"-Zi", L"-Qembed_debug",
        L"-Od",
        L"-Zpr"
    };

    ComPtr<IDxcResult> shader_result;
    if (FAILED(dxc_compiler_->Compile(
        &shader_source_buffer,
        arguments,
        _countof(arguments),
        include_handler_.Get(),
        IID_PPV_ARGS(&shader_result)))) {
        OutputDebugStringA("Failed compile shader\n");
        assert(false);
        return {};
    }

    ComPtr<IDxcBlobUtf8> shader_error = nullptr;
    shader_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shader_error), nullptr);
    if (shader_error != nullptr && shader_error->GetStringLength() != 0) {
        std::string error_msg = shader_error->GetStringPointer();
        OutputDebugStringA(error_msg.c_str());
        assert(false);
        return {};
    }

    std::shared_ptr<Shader> shader(new Shader);
    if (FAILED(shader_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader->blob), nullptr))) {
        OutputDebugStringA("Failed get blob\n");
        assert(false);
        return {};
    }

    if (is_output_compiled_file) {
        std::filesystem::path output_path = full_path;
        output_path.replace_extension(L".cso");
        std::ofstream output_file(output_path, std::ios::binary);
        if (output_file) {
            output_file.write(reinterpret_cast<const char*>(shader->blob->GetBufferPointer()), shader->blob->GetBufferSize());
            OutputDebugStringA("Output compiled shader object\n");
        }
    }

    OutputDebugStringW(std::format(L"Compile Succeeded, path:{}, profile:{}\n", full_path.wstring(), profile).c_str());
    return shader;
}

std::shared_ptr<Shader> ShaderManager::LoadShader(const std::filesystem::path& file_name) {
    assert(file_name.extension() == ".cso");

    auto full_path = directory_ / file_name;
    OutputDebugStringW(std::format(L"Begin LoadShader, path:{}\n", full_path.wstring()).c_str());

    ComPtr<IDxcBlobEncoding> shader_source;
    if (FAILED(dxc_utils_->LoadFile(full_path.c_str(), nullptr, &shader_source))) {
        OutputDebugStringA("Failed load shader file\n");
        assert(false);
        return {};
    }

    std::shared_ptr<Shader> shader(new Shader);
    shader->blob = shader_source.Get();

    OutputDebugStringW(std::format(L"Loaded Succeeded, path:{}\n", full_path.wstring()).c_str());
    return shader;
}

void ShaderManager::Initialize() {
    if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxc_utils_)))) {
        OutputDebugStringA("Failed create dxc utils\n");
        std::abort();
    }
    if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxc_compiler_)))) {
        OutputDebugStringA("Failed create dxc compiler\n");
        std::abort();
    }
    if (FAILED(dxc_utils_->CreateDefaultIncludeHandler(&include_handler_))) {
        OutputDebugStringA("Failed create include handler\n");
        std::abort();
    }
    directory_ = std::filesystem::current_path();
}
