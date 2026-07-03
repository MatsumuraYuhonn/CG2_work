#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Sprite.h"


struct DirectionalLight {
    Vector4 color;
    Vector3 direction;
    float intensity;
};


class ConstantBuffer {
public:
    ConstantBuffer();
    virtual ~ConstantBuffer();

    ConstantBuffer(const ConstantBuffer&) = delete;
    ConstantBuffer& operator=(const ConstantBuffer&) = delete;

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

    ID3D12Resource* GetResource() const;

protected:

    bool InitializeInternal(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes);
    void Unmap();

    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    void* mappedData_; 
};


struct Material;

class MaterialConstantBuffer : public ConstantBuffer {
public:
    bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device);

    Material* operator->() { return reinterpret_cast<Material*>(mappedData_); }
    const Material* operator->() const { return reinterpret_cast<const Material*>(mappedData_); }
};

struct TransformationMatrix;

class TransformationMatrixConstantBuffer : public ConstantBuffer {
public:
    bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device);

    TransformationMatrix* operator->() { return reinterpret_cast<TransformationMatrix*>(mappedData_); }
    const TransformationMatrix* operator->() const { return reinterpret_cast<const TransformationMatrix*>(mappedData_); }
};

struct DirectionalLight;

class DirectionalLightConstantBuffer : public ConstantBuffer {
public:
    bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device);

    DirectionalLight* operator->() { return reinterpret_cast<DirectionalLight*>(mappedData_); }
    const DirectionalLight* operator->() const { return reinterpret_cast<const DirectionalLight*>(mappedData_); }
};