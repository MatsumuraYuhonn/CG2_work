#include "ConstantBuffer.h"
#include "Vector.h" 
#include <cassert>

ConstantBuffer::ConstantBuffer() : mappedData_(nullptr) {}

ConstantBuffer::~ConstantBuffer() {
    Unmap();
}

bool ConstantBuffer::InitializeInternal(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes) {

    size_t alignedSize = (sizeInBytes + 255) & ~255;

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = alignedSize;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&resource_)
    );
    if (FAILED(hr)) return false;

    hr = resource_->Map(0, nullptr, &mappedData_);
    return SUCCEEDED(hr);
}

void ConstantBuffer::Unmap() {
    if (resource_ && mappedData_) {
        resource_->Unmap(0, nullptr);
        mappedData_ = nullptr;
    }
}

D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetGPUVirtualAddress() const {
    return resource_ ? resource_->GetGPUVirtualAddress() : 0;
}

ID3D12Resource* ConstantBuffer::GetResource() const {
    return resource_.Get();
}



bool MaterialConstantBuffer::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device) {
    return InitializeInternal(device, sizeof(Material));
}

bool TransformationMatrixConstantBuffer::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device) {
    return InitializeInternal(device, sizeof(TransformationMatrix));
}

bool DirectionalLightConstantBuffer::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device) {
    return InitializeInternal(device, sizeof(DirectionalLight));
}