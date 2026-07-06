#include "DescriptorHeapManager.h"
#include <cassert>

void DescriptorHeapManager::Initialize(
    Microsoft::WRL::ComPtr<ID3D12Device> device,
    D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    UINT numDescriptors,
    bool shaderVisible)
{
    assert(device != nullptr);

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = heapType;
    desc.NumDescriptors = numDescriptors;
    desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap_));
    assert(SUCCEEDED(hr));

    // デバイスからこのヒープタイプのハンドルインクリメントサイズを取得
    descriptorSize_ = device->GetDescriptorHandleIncrementSize(heapType);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetCPUDescriptorHandle(uint32_t index) const {
    assert(descriptorHeap_ != nullptr);
    D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += (static_cast<size_t>(descriptorSize_) * index);
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetGPUDescriptorHandle(uint32_t index) const {
    assert(descriptorHeap_ != nullptr);
    D3D12_GPU_DESCRIPTOR_HANDLE handle = descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += (static_cast<size_t>(descriptorSize_) * index);
    return handle;
}