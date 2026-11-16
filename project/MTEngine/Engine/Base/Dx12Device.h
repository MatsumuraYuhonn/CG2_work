#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstdint>
#include <cassert>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")


//DirectX12のデバイス、コマンドキュー、メモリ管理等の初期化・管理を行うクラス

namespace MTEngine {

	class Dx12Device {
	public:
		Dx12Device() = default;
		~Dx12Device();

		// デバイス、コマンドキュー、スワップチェーン等の基本初期化
		bool Initialize();

		// GPU側の処理が完了するまでCPUを待機させる
		void WaitForGPU();

		// 各種DirectXリソースのゲッター
		Microsoft::WRL::ComPtr<ID3D12Device> GetDevice()const { return device_; }
		Microsoft::WRL::ComPtr<IDXGIFactory7> GetDxgiFactory() const { return dxgiFactory_; }
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const { return commandAllocator_; }
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const { return commandQueue_; }
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() const { return commandList_; }

		// デスクリプタサイズ（メモリのアライメントやオフセット計算に使用）
		UINT GetDescriptorSizeSRV() const { return descriptorSizeSRV_; }
		UINT GetDescriptorSizeRTV() const { return descriptorSizeRTV_; }
		UINT GetDescriptorSizeDSV() const { return descriptorSizeDSV_; }

		// バッファリソースの生成
		static Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* pDevice, size_t sizeInBytes);


		// デスクリプタヒープの生成
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);


	private:
		Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Device>  device_ = nullptr;

		// コマンド実行管理
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;

		// GPU同期用フェンス
		Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
		uint32_t fenceValue_ = 0;
		HANDLE fenceEvent_ = nullptr;

		// 記述子サイズ（各ヒープタイプで必要なアライメント）
		UINT descriptorSizeSRV_ = 0;
		UINT descriptorSizeRTV_ = 0;
		UINT descriptorSizeDSV_ = 0;
	};

}