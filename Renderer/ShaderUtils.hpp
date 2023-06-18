#pragma once

#include <d3d12.h>
#include <dxcapi.h>
#include <wrl.h>

#include <memory>
#include <string>
#include <filesystem>

class Shader {
public:
    operator D3D12_SHADER_BYTECODE() const { return { blob->GetBufferPointer(), blob->GetBufferSize() }; }

    bool IsEnabled() const { return blob; }

private:
    Shader() = default;

    Microsoft::WRL::ComPtr<IDxcBlob> blob;

    friend class ShaderManager;
};

class ShaderManager {
public:
    [[nodiscard]]
    std::shared_ptr<Shader> CompileShader(const std::filesystem::path& file_name, const std::wstring& profile, bool is_output_compiled_file = true);
    [[nodiscard]]
    std::shared_ptr<Shader> LoadShader(const std::filesystem::path& file_name);

    void Initialize();

    void SetDirectory(const std::filesystem::path& directory) { directory_ = directory; }

private:
    Microsoft::WRL::ComPtr<IDxcUtils> dxc_utils_;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxc_compiler_;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> include_handler_;
    std::filesystem::path directory_;
};