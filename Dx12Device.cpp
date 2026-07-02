#include "Dx12Device.h"
#include "Logger.h"
#include <cassert>
#include <format>

bool Dx12Device::Initialize() {

    HRESULT hr = S_OK;

    hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory_));
 
    assert(SUCCEEDED(hr));
 
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
   

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
    };

    const char* featureLevelStrings[] = {
        "12.2", "12.1", "12.0",
    };

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

    Log("complete create D3D12Device!!!\n");
    return true;
}

