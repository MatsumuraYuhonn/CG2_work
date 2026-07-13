#include "Dx12Device.h"
#include "MTEngine/Engine/Debug/Logger.h"
#include <cassert>
#include <format>


namespace MTEngine {

    Dx12Device::~Dx12Device() {
        if (fenceEvent_) {
            CloseHandle(fenceEvent_);
        }
    }

    bool Dx12Device::Initialize() {

        HRESULT hr = S_OK;

        // --- DXGIファクトリの生成 ---
        hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory_));
        assert(SUCCEEDED(hr));

        // --- アダプターの選定 ---
        for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter_)) != DXGI_ERROR_NOT_FOUND; i++) {
            DXGI_ADAPTER_DESC3 adapterDesc{};
            hr = useAdapter_->GetDesc3(&adapterDesc);
            assert(SUCCEEDED(hr));

            if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
                Log(ConvertString(std::format(L"Use Adapter! {}\n", adapterDesc.Description)));
                break;
            }
            useAdapter_ = nullptr;
        }
        assert(useAdapter_ != nullptr);

        // --- デバイスの生成 ---
        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_12_2,
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
        };

        const char* featureLevelStrings[] = { "12.2", "12.1", "12.0" };
        bool deviceCreated = false;
        for (size_t i = 0; i < std::size(featureLevels); ++i) {
            hr = D3D12CreateDevice(useAdapter_.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
            if (SUCCEEDED(hr)) {
                Log(std::format("Feature Level : {}\n", featureLevelStrings[i]));
                deviceCreated = true;
                break;
            }
        }
        assert(deviceCreated);

        // --- 記述子サイズの取得と保持 ---
        descriptorSizeSRV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        descriptorSizeRTV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        descriptorSizeDSV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        // --- コマンド周りの生成 ---
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        hr = device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_));
        assert(SUCCEEDED(hr));

        hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
        assert(SUCCEEDED(hr));

        hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
        assert(SUCCEEDED(hr));


        hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
        assert(SUCCEEDED(hr));

        fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
        assert(fenceEvent_ != nullptr);

        Log("complete create D3D12Device, Commands and Fence!!!\n");
        return true;
    }

    void Dx12Device::WaitForGPU() {
        fenceValue_++;

        commandQueue_->Signal(fence_.Get(), fenceValue_);

        if (fence_->GetCompletedValue() < fenceValue_) {

            fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
            WaitForSingleObject(fenceEvent_, INFINITE);
        }
    }

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Dx12Device::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
        D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
        descriptorHeapDesc.Type = heapType;
        descriptorHeapDesc.NumDescriptors = numDescriptors;
        descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        HRESULT hr = device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
        assert(SUCCEEDED(hr));
        return descriptorHeap;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> Dx12Device::CreateBufferResource(
        ID3D12Device* pDevice,
        size_t sizeInBytes
    ) {
        assert(pDevice != nullptr && "Device pointer is null.");

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resDesc.Width = sizeInBytes;
        resDesc.Height = 1;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels = 1;
        resDesc.Format = DXGI_FORMAT_UNKNOWN;
        resDesc.SampleDesc.Count = 1;
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        Microsoft::WRL::ComPtr<ID3D12Resource> pBufferResource;
        HRESULT hr = pDevice->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&pBufferResource)
        );

        // assert で成否を確認
        assert(SUCCEEDED(hr) && "Failed to create D3D12 committed resource.");

        return pBufferResource;
    }

}