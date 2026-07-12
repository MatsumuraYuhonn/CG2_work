#pragma once
#include <windows.h>
#include <dxcapi.h>
#include <wrl.h>
#include <string>
#include <cassert>

// DirectX Shader Compiler (DXC) を使用してシェーダーファイルをコンパイルするクラス
class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler();

    // DXCインターフェースの初期化
    bool Initialize();

    // シェーダーファイルをコンパイルする
    Microsoft::WRL::ComPtr<IDxcBlob> Compile(
        const std::wstring& filePath,
        const wchar_t* profile
    );

private:
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_ = nullptr;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_ = nullptr;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_ = nullptr;
};