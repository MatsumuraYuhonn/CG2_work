#include "MTEngine/Engine/Base/Engine.h"

//Windowsアプリケーションのエントリーポイント
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    //エンジン本体の生成
    std::unique_ptr<Engine> engine = std::make_unique<Engine>();

    //エンジンの初期化
    engine->Initialize();
    //メインループの実行
    engine->Run();
    //エンジンの終了処理
    engine->Finalize();

    return 0;
}