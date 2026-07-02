#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")


class Dx12Device {
public:
	Dx12Device() = default;
	~Dx12Device() = default;

	bool Initialize();

	ID3D12Device* GetDevice() const { return device_.Get(); }
	IDXGIFactory7* GetDxgiFactory() const { return dxgiFactory_.Get(); }


private:
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Device>  device_ = nullptr;
};


