#include "DirectionalLight.h"
#include <cassert>

void DirectionalLight::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device) {

    // 256バイトアライメント
    size_t sizeInBytes = (sizeof(ConstBufferData) + 255) & ~255;

    D3D12_HEAP_PROPERTIES uploadHeapProperties{};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferResourceDesc{};
    bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferResourceDesc.Width = sizeInBytes;
    bufferResourceDesc.Height = 1;
    bufferResourceDesc.DepthOrArraySize = 1;
    bufferResourceDesc.MipLevels = 1;
    bufferResourceDesc.SampleDesc.Count = 1;
    bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&constBuffer_)
    );
    assert(SUCCEEDED(hr));

    // 初期値の設定
    data.color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    data.direction = Vector3(0.0f, -1.0f, 1.0f);
    data.intensity = 1.0f;

    // マップしてポインタを保持しておく（常時マップ）
    hr = constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_));
    assert(SUCCEEDED(hr));

    Update();
}

void DirectionalLight::Update() {
    if (mappedData_) {
        *mappedData_ = data;
    }
}

void DirectionalLight::Bind(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT rootParameterIndex) {
    commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, GetGPUVirtualAddress());
}