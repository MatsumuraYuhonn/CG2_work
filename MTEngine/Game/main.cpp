#include "MTEngine/Engine/Base/Engine.h"

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    std::unique_ptr<Engine> engine = std::make_unique<Engine>();

    engine->Initialize();
    engine->Run();
    engine->Finalize();

    return 0;
}