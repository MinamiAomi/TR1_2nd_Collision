#include "DX12.h"

namespace CG::DX12 {

    void DynamicBuffer::Initialize(const Device& device, size_t bufferSize, bool allowUnorderedAccess, CG::DX12::Resource::State initialState) {
        bufferSize_ = bufferSize;
        resource_.InitializeForBuffer(
            device, 
            bufferSize_, 
            initialState, 
            CG::DX12::Resource::HeapType::Upload, 
            allowUnorderedAccess ? CG::DX12::Resource::Flag::AllowUnorderedAccess : CG::DX12::Resource::Flag::None);
        dataBegin_ = resource_.Map();
        memset(dataBegin_, 0, bufferSize_);
    }

}