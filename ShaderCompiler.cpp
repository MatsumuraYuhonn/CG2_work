#include "ShaderCompiler.h"
#include "Logger.h"
#include <cassert>
#include <format>

ShaderCompiler::ShaderCompiler() {
   
}

ShaderCompiler::~ShaderCompiler() {
    
}

bool ShaderCompiler::Initialize() {
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
    if (FAILED(hr)) return false;

    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
    if (FAILED(hr)) return false;

    hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
    if (FAILED(hr)) return false;

    return true;
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::Compile(const std::wstring& filePath, const wchar_t* profile) {
    Log(ConvertString(std::format(L"Begin CompileShader, Path:{}, profile:{}\n", filePath, profile)));

    Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
    HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
    assert(SUCCEEDED(hr));

    DxcBuffer shaderSourceBuffer{};
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8;

    // 引数の設定
    LPCWSTR arguments[] = {
        filePath.c_str(),
        L"-E", L"main",
        L"-T", profile,
        L"-Zi", L"-Qembed_debug",
        L"-Od",
        L"-Zpr"
    };

    Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
    hr = dxcCompiler_->Compile(
        &shaderSourceBuffer,
        arguments,
        _countof(arguments),
        includeHandler_.Get(),
        IID_PPV_ARGS(&shaderResult)
    );
    assert(SUCCEEDED(hr));

    // エラー確認
    Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
    if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
        Log(shaderError->GetStringPointer());
        assert(false);
    }

    // 結果の受け取り
    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hr));

    Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));

    return shaderBlob;
}