workspace "DirectXGame"
    architecture "x64" -- スクリーンショットよりx64をターゲットに設定
    configurations { "Debug", "Release" } -- 一般的な構成（必要に応じて追加してください）

    -- 出力先ディレクトリの共通変数定義
    -- $(OutDir) や $(IntDir) に相当するパスを設定します
    targetdir ("bin/" .. "%{cfg.buildcfg}")
    objdir ("bin-int/" .. "%{cfg.buildcfg}")

-- プロジェクトの定義
project "DirectXGame"
    kind "WindowedApp" -- Windowsアプリケーション (mainではなくWinMain等の場合)
    language "C++"
    cppdialect "C++20" -- 使用しているC++のバージョンに合わせて変更してください（C++17やC++20など）
    staticruntime "Off"

    -- ① 対象となるソースコード・ヘッダーファイルの追加
    -- プロジェクト配下、および MTEngine 内のすべての cpp/h ファイルを対象にします
    files {
        "*.h", "*.cpp",
        "MTEngine/Engine/**.h", "MTEngine/Engine/**.cpp",
        "MTEngine/Game/**.h", "MTEngine/Game/**.cpp"
    }

    -- ② 追加のインクルードディレクトリ (C/C++ -> 全般)
    -- 添付画像にあった $(ProjectDir)（＝現在のプロジェクト構成）に相当するもの、
    -- およびソースを読み込むために必要なディレクトリを指定します
    includedirs {
        ".", -- $(ProjectDir) に相当
        "MTEngine/Engine",
        "MTEngine/Game"
        -- externals 内に利用するライブラリのヘッダがあればここに追加します
    }

    -- ③ 追加の依存ファイル (リンカー -> 入力) と ライブラリディレクトリ
    -- DirectX12のビルドに必要な標準的なライブラリ群を指定しています
    links {
        "d3d12",
        "dxgi",
        "dxguid",
        "D3DCompiler"
    }

    -- 構成ごとの個別設定
    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On" -- デバッグ情報の形式などを有効に

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On" -- 最適化を有効に