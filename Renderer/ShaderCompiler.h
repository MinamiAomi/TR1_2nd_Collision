#pragma once

#include <dxcapi.h>
#include <wrl/client.h>

namespace CG::DX12 {
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    class ShaderCompiler {
    public:
        void Initialize();
        void Compile();

    private:
        Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
        Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
        Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
    };

}