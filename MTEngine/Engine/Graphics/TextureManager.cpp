#include "TextureManager.h"
#include "DescriptorHeapManager.h"
#include "externals/DirectXTex/d3dx12.h"
#include <cassert>
#include <utility>

namespace MTEngine {

    // main.cpp に定義されている想定の文字列変換関数
    extern std::wstring ConvertString(const std::string& str);
    extern Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes);

    void TextureManager::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, DescriptorHeapManager* srvHeapManager) {
        assert(device);
        assert(srvHeapManager);
        device_ = device;
        srvHeapManager_ = srvHeapManager;
    }

    bool TextureManager::Load(const std::string& filePath, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
        if (!device_ || !srvHeapManager_ || !commandList || filePath.empty()) {
            return false;
        }

        // 既に読み込み済みならスキップ
        if (textures_.find(filePath) != textures_.end()) {
            return true;
        }

        // 1. ファイル読み込み
        DirectX::ScratchImage mipImages;
        if (!LoadTextureFile(filePath, mipImages)) {
            return false;
        }
        const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

        // 2. リソース作成
        Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = CreateTextureResource(metadata);
        if (!textureResource) {
            return false;
        }

        // 3. データの転送
        Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = UploadTextureData(textureResource, mipImages, commandList);
        if (!intermediateResource) {
            return false;
        }
        intermediateResources_.push_back(intermediateResource); // コマンド実行完了まで保持

        // 4. 共通SRVヒープから空き番号を確保してSRVを作成する。
        uint32_t index = 0;
        if (!srvHeapManager_->AllocateDescriptor(index)) {
            return false;
        }
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU = srvHeapManager_->GetCPUDescriptorHandle(index);
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU = srvHeapManager_->GetGPUDescriptorHandle(index);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = metadata.format;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = static_cast<UINT>(metadata.mipLevels);

        device_->CreateShaderResourceView(textureResource.Get(), &srvDesc, srvHandleCPU);

        // コンテナに追加
        TextureData data;
        data.resource = textureResource;
        data.metadata = metadata;
        data.srvHandleCPU = srvHandleCPU;
        data.srvHandleGPU = srvHandleGPU;

        textures_[filePath] = data;
        return true;
    }

    bool TextureManager::TryGetGPUDescriptorHandle(
        const std::string& filePath,
        D3D12_GPU_DESCRIPTOR_HANDLE& handle) const {

        auto it = textures_.find(filePath);
        if (it == textures_.end()) {
            return false;
        }

        handle = it->second.srvHandleGPU;
        return true;
    }

    bool TextureManager::LoadTextureFile(
        const std::string& filePath,
        DirectX::ScratchImage& mipImages) {

        DirectX::ScratchImage image{};
        std::wstring filePathw = ConvertString(filePath);
        HRESULT hr = DirectX::LoadFromWICFile(filePathw.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
        if (FAILED(hr)) {
            return false;
        }

        DirectX::ScratchImage mipImage{};
        hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImage);
        if (SUCCEEDED(hr)) {
            mipImages = std::move(mipImage);
        }
        else {
            // ミップ生成に対応していない画像でも、元画像は使用できる。
            mipImages = std::move(image);
        }

        return true;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateTextureResource(const DirectX::TexMetadata& metadata) {
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Width = UINT(metadata.width);
        resourceDesc.Height = UINT(metadata.height);
        resourceDesc.MipLevels = UINT16(metadata.mipLevels);
        resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
        resourceDesc.Format = metadata.format;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

        Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
        HRESULT hr = device_->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&resource)
        );
        if (FAILED(hr)) {
            return nullptr;
        }

        return resource;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::UploadTextureData(
        Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
    {
        std::vector<D3D12_SUBRESOURCE_DATA> subresources;
        DirectX::PrepareUpload(device_.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
        if (subresources.empty()) {
            return nullptr;
        }
        uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));

        Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(device_, intermediateSize);
        UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = texture.Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
        commandList->ResourceBarrier(1, &barrier);

        return intermediateResource;
    }

}
