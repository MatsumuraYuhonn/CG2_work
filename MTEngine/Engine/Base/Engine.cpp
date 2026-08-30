#include "Engine.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numbers>

namespace MTEngine {
    namespace {
        constexpr char kTitleBgmPath[] =
            "MTEngine/Assets/Resources/Audio/BGM/Title.mp3";
        constexpr char kPhase1BgmPath[] =
            "MTEngine/Assets/Resources/Audio/BGM/phase1.mp3";
        constexpr char kPhase2BgmPath[] =
            "MTEngine/Assets/Resources/Audio/BGM/phase2.mp3";
        constexpr char kGameClearBgmPath[] =
            "MTEngine/Assets/Resources/Audio/BGM/GameClear.mp3";
        constexpr char kGameOverBgmPath[] =
            "MTEngine/Assets/Resources/Audio/BGM/GameOver.mp3";
        constexpr char kSelectSoundPath[] =
            "MTEngine/Assets/Resources/Audio/SFX/select.mp3";
        constexpr char kNightmareSoundPath[] =
            "MTEngine/Assets/Resources/Audio/SFX/nightmare.mp3";
        constexpr char kShotSoundPath[] =
            "MTEngine/Assets/Resources/Audio/SFX/Shot.mp3";
        constexpr char kChargeSoundPath[] =
            "MTEngine/Assets/Resources/Audio/SFX/Charge.mp3";
        constexpr char kChargeShotSoundPath[] =
            "MTEngine/Assets/Resources/Audio/SFX/Charge_Shot.mp3";
        constexpr char kTeleportSoundPath[] =
            "MTEngine/Assets/Resources/Audio/SFX/teleport.mp3";
        constexpr char kDashSoundPath[] =
            "MTEngine/Assets/Resources/Audio/SFX/dash.mp3";
        constexpr char kAlertSoundPath[] =
            "MTEngine/Assets/Resources/Audio/SFX/alert.mp3";
        constexpr char kFallSoundPath[] =
            "MTEngine/Assets/Resources/Audio/SFX/fall.mp3";
        constexpr char kDamageSoundPath[] =
            "MTEngine/Assets/Resources/Audio/SFX/damage.mp3";
        constexpr char kEnemyDamageSoundPath[] =
            "MTEngine/Assets/Resources/Audio/SFX/enemyDamage.mp3";
        constexpr float kBgmVolume = 0.35f;
        constexpr float kEnemyMovementSoundVolume = 0.65f;
        constexpr float kDamageSoundVolume = 0.48f;
        constexpr float kShotVolume = 0.35f;
        constexpr float kChargeVolume = 0.30f;
        constexpr float kCompletedChargeVolume = 0.15f;
        constexpr float kChargeShotVolume = 0.40f;
        constexpr float kMasterVolume = 0.65f;
        constexpr float kBgmCrossfadeDuration = 1.5f;
        constexpr float kDefaultCameraDistance = 79.0f;
        constexpr float kPhase1IntroCameraDistance = 45.0f;
        constexpr float kPhase2IntroCameraDistance = 52.0f;
        constexpr float kEnemyDefeatCameraDistance = 52.0f;

        bool ConfigureGameWorkingDirectory() {
            std::array<wchar_t, 32768> executablePathBuffer{};
            const DWORD executablePathLength = GetModuleFileNameW(
                nullptr,
                executablePathBuffer.data(),
                static_cast<DWORD>(executablePathBuffer.size()));
            if (executablePathLength == 0 ||
                executablePathLength >= executablePathBuffer.size()) {
                return false;
            }

            const std::filesystem::path executableDirectory =
                std::filesystem::path(
                    std::wstring(
                        executablePathBuffer.data(),
                        executablePathLength)).parent_path();
            std::error_code error;
            const std::filesystem::path currentDirectory =
                std::filesystem::current_path(error);
            const std::array<std::filesystem::path, 5> candidates = {
                currentDirectory,
                executableDirectory,
                executableDirectory.parent_path(),
                executableDirectory.parent_path().parent_path(),
                executableDirectory.parent_path().parent_path().parent_path(),
            };

            for (const std::filesystem::path& candidate : candidates) {
                if (candidate.empty()) {
                    continue;
                }
                const std::filesystem::path resourceDirectory =
                    candidate / "MTEngine" / "Assets" / "Resources";
                const std::filesystem::path shaderDirectory =
                    candidate / "MTEngine" / "Assets" / "Shaders";
                if (!std::filesystem::is_directory(resourceDirectory, error) ||
                    !std::filesystem::is_directory(shaderDirectory, error)) {
                    continue;
                }

                std::filesystem::current_path(candidate, error);
                return !error;
            }
            return false;
        }
    }

    // グローバル関数の実装
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes) {

        D3D12_HEAP_PROPERTIES uploadHeapProperties{};
        uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC bufferResourceDesc{};
        bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferResourceDesc.Width = sizeInBytes;
        bufferResourceDesc.Height = 1;
        bufferResourceDesc.DepthOrArraySize = 1;
        bufferResourceDesc.MipLevels = 1;
        bufferResourceDesc.SampleDesc.Count = 1;
        bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource = nullptr;
        HRESULT hr = device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferResourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&bufferResource)
        );
        assert(SUCCEEDED(hr));

        return bufferResource;
    }

    void Engine::Initialize() {
        if (!ConfigureGameWorkingDirectory()) {
            MessageBoxW(
                nullptr,
                L"MTEngine/Assets が見つかりません。\n"
                L"DirectXGame.exe と MTEngine フォルダーの配置を確認してください。",
                L"DirectXGame - Resource Error",
                MB_OK | MB_ICONERROR);
            ExitProcess(EXIT_FAILURE);
        }

        HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
        assert(SUCCEEDED(hr));

        SetUnhandledExceptionFilter(ExportDump);
        InitializeLogger();
        createLogFile();
        Log("String\n");

        hwnd_ = CreateGameWindow();

        input_ = std::make_unique<Input>();
        input_->Initialize(GetModuleHandle(nullptr), hwnd_);

#ifdef _DEBUG
        Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
            debugController->SetEnableGPUBasedValidation(TRUE);
        }
#endif

        // DirectX12の初期化
        bool isInitialized = dx12Device_.Initialize();
        assert(isInitialized);

        auto device = dx12Device_.GetDevice();
        auto commandList = dx12Device_.GetCommandList();
        auto commandQueue = dx12Device_.GetCommandQueue();
        auto dxgiFactory = dx12Device_.GetDxgiFactory();

#ifdef _DEBUG
        Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
        if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
            D3D12_MESSAGE_ID denyIds[] = { D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE };
            D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
            D3D12_INFO_QUEUE_FILTER filter{};
            filter.DenyList.NumIDs = _countof(denyIds);
            filter.DenyList.pIDList = denyIds;
            filter.DenyList.NumSeverities = _countof(severities);
            filter.DenyList.pSeverityList = severities;
            infoQueue->PushStorageFilter(&filter);
        }
#endif

        // スワップチェーンの作成
        bool isSwapChainInit = swapChain_.Initialize(dxgiFactory, commandQueue, hwnd_, kClientWidth, kClientHeight);
        assert(isSwapChainInit);

        // ShaderCompilerの初期化
        shaderCompiler_ = std::make_unique<ShaderCompiler>();
        bool isShaderCompilerInit = shaderCompiler_->Initialize();
        assert(isShaderCompilerInit);

        // PipelineManagerの初期化
        pipelineManager_ = std::make_unique<PipelineManager>();
        pipelineManager_->Initialize(device, shaderCompiler_.get());

        // Rendererの初期化
        renderer_ = std::make_unique<Renderer>();
        renderer_->Initialize(&dx12Device_, &swapChain_, pipelineManager_.get(), kClientWidth, kClientHeight);

        audioManager_ = std::make_unique<AudioManager>();
        audioManager_->Initialize();
        audioManager_->SetMasterVolume(kMasterVolume);

        // ImGuiManagerの初期化
        imGuiManager_ = std::make_unique<ImGuiManager>();
        imGuiManager_->Initialize(
            hwnd_,
            device.Get(),
            swapChain_.GetBufferCount(),
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            renderer_->GetSrvHeapManager()->GetHeap(),
            renderer_->GetSrvHeapManager()->GetCPUDescriptorHandle(0),
            renderer_->GetSrvHeapManager()->GetGPUDescriptorHandle(0)
        );

        debugCamera_.Initialize();

        const bool isGameSceneInitialized = gameScene_.Initialize(
            device,
            commandList,
            renderer_->GetSrvHeapManager(),
            "MTEngine/Assets/Resources/Data/Stages/stage1.csv");
        assert(isGameSceneInitialized);

        editor_ = std::make_unique<Editor>();
        editor_->Initialize(
            renderer_->GetSceneTextureHandle(), &gameScene_, &debugCamera_, &sceneManager_);

        PlayBgm(kTitleBgmPath);

    }

    void Engine::Run() {
        MSG msg{};
        while (msg.message != WM_QUIT) {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else {
                Update();
                Draw();
            }
        }
    }

    void Engine::Update() {
        if (input_) {
            input_->Update();
        }

        // ImGuiのフレーム開始
        imGuiManager_->BeginFrame();

        editor_->Update();
        sceneManager_.Update(input_.get(), gameScene_);
        gameScene_.UpdateSkyDome();
        gameScene_.SetTutorialSkipHoldProgress(
            sceneManager_.GetTutorialSkipHoldProgress());
        if (sceneManager_.IsPlayableScene()) {
            gameScene_.SetSelectedCellIndex(editor_->GetSelectedTileIndex());
            gameScene_.UpdatePlayer(input_.get());
        }
        UpdateAudio();
        const SceneType currentCameraScene = sceneManager_.GetCurrentScene();
        if (currentCameraScene == SceneType::Tutorial &&
            previousCameraScene_ != SceneType::Tutorial) {
            debugCamera_.SetTarget({ 0.0f, 0.0f, 2.134f });
            debugCamera_.SetDistance(kDefaultCameraDistance);
        }
        const bool isPhase1IntroActive =
            sceneManager_.IsGameScene() && gameScene_.IsPhase1IntroActive();
        const bool isPhase2IntroActive =
            sceneManager_.IsGameScene() && gameScene_.IsPhase2IntroActive();
        const bool isEnemyDefeatAnimationActive =
            sceneManager_.IsGameScene() &&
            gameScene_.IsEnemyDefeatAnimationActive();
        if (isPhase1IntroActive) {
            debugCamera_.SetTarget(gameScene_.GetPhase1IntroCameraTarget());
            const float pullBackProgress =
                gameScene_.GetPhase1IntroCameraPullBackProgress();
            debugCamera_.SetDistance(
                kPhase1IntroCameraDistance +
                (kDefaultCameraDistance - kPhase1IntroCameraDistance) *
                    pullBackProgress);
        }
        else if (isPhase2IntroActive) {
            debugCamera_.SetTarget(gameScene_.GetPhase2IntroCameraTarget());
            const float pullBackProgress =
                gameScene_.GetPhase2IntroCameraPullBackProgress();
            debugCamera_.SetDistance(
                kPhase2IntroCameraDistance +
                (kDefaultCameraDistance - kPhase2IntroCameraDistance) *
                    pullBackProgress);
        }
        else if (isEnemyDefeatAnimationActive) {
            debugCamera_.SetTarget(gameScene_.GetEnemyDefeatCameraTarget());
            const float pullBackProgress =
                gameScene_.GetEnemyDefeatCameraPullBackProgress();
            debugCamera_.SetDistance(
                kEnemyDefeatCameraDistance +
                (kDefaultCameraDistance - kEnemyDefeatCameraDistance) *
                    pullBackProgress);
        }
        else if (wasPhase1IntroActive_ || wasPhase2IntroActive_ ||
            wasEnemyDefeatAnimationActive_) {
            debugCamera_.SetTarget({ 0.0f, 0.0f, 2.134f });
            debugCamera_.SetDistance(kDefaultCameraDistance);
        }
        wasPhase1IntroActive_ = isPhase1IntroActive;
        wasPhase2IntroActive_ = isPhase2IntroActive;
        wasEnemyDefeatAnimationActive_ =
            isEnemyDefeatAnimationActive;
        debugCamera_.Update(
            input_.get(),
            editor_->IsSceneViewFocused() &&
                !isPhase1IntroActive &&
                !isPhase2IntroActive &&
                !isEnemyDefeatAnimationActive);
        previousCameraScene_ = currentCameraScene;

        imGuiManager_->EndFrame();
    }

    void Engine::Draw() {
        // フレーム開始処理 (クリア、バリア遷移、ビューポート設定、パイプラインバインド等)
        renderer_->BeginFrame();

        auto commandList = renderer_->GetCommandList();

        renderer_->BeginScene();
        if (sceneManager_.GetCurrentScene() == SceneType::Title) {
            gameScene_.DrawSkyDome(
                commandList,
                debugCamera_.GetViewMatrix(),
                debugCamera_.GetProjectionMatrix());
            if (sceneManager_.IsTitleRankingOpen()) {
                gameScene_.DrawRankingScreen(commandList);
            }
            else {
                gameScene_.DrawTitleOverlay(commandList);
            }
        }
        else if (sceneManager_.IsPlayableScene()) {
            gameScene_.Draw(
                commandList,
                debugCamera_.GetViewMatrix(),
                debugCamera_.GetProjectionMatrix(),
                sceneManager_.GetElapsedGameTimeSeconds(),
                sceneManager_.IsGameScene());
        }
        else if (sceneManager_.GetCurrentScene() == SceneType::Clear ||
            sceneManager_.GetCurrentScene() == SceneType::Miss) {
            const float elapsedGameTimeSeconds =
                sceneManager_.GetElapsedGameTimeSeconds();
            gameScene_.Draw(
                commandList,
                debugCamera_.GetViewMatrix(),
                debugCamera_.GetProjectionMatrix(),
                elapsedGameTimeSeconds,
                false);
            renderer_->ClearSceneDepth();
            gameScene_.DrawResultScreen(
                commandList,
                elapsedGameTimeSeconds,
                sceneManager_.GetCurrentScene() == SceneType::Clear,
                sceneManager_.IsGoNightmareTipOpen());
        }
        renderer_->EndScene();

#ifdef USE_IMGUI
        // ImGuiの描画
        imGuiManager_->Draw(commandList);
#else
        renderer_->CopySceneToBackBuffer();
#endif

        // フレーム終了処理 (バリア遷移、ExecuteCommandLists、Present、Wait)
        renderer_->EndFrame();
    }

    void Engine::Finalize() {
        chargeAudio_.reset();
        outgoingBgmAudio_.reset();
        bgmAudio_.reset();
        soundEffects_.clear();

        if (audioManager_) {
            audioManager_->Finalize();
            audioManager_.reset();
        }

        editor_.reset();

        if (imGuiManager_) {
            imGuiManager_->Finalize();
            imGuiManager_.reset();
        }

        CoUninitialize();
        CloseWindow(hwnd_);
    }

    void Engine::PlayBgm(const std::string& filePath) {
        if (!audioManager_ || filePath == currentBgmPath_) {
            return;
        }

        const bool shouldCrossfadeBossPhase =
            currentBgmPath_ == kPhase1BgmPath &&
            filePath == kPhase2BgmPath && bgmAudio_;
        if (shouldCrossfadeBossPhase) {
            outgoingBgmAudio_ = std::move(bgmAudio_);
            currentBgmPath_ = filePath;
            bgmAudio_ = audioManager_->Play(filePath, true, 0.0f);
            bgmCrossfadeElapsedTime_ = 0.0f;
            isBgmCrossfading_ = true;
            previousBgmUpdateTime_ = std::chrono::steady_clock::now();
            return;
        }

        outgoingBgmAudio_.reset();
        isBgmCrossfading_ = false;
        bgmCrossfadeElapsedTime_ = 0.0f;
        bgmAudio_.reset();
        currentBgmPath_ = filePath;
        if (!filePath.empty()) {
            bgmAudio_ = audioManager_->Play(filePath, true, kBgmVolume);
        }
    }

    void Engine::UpdateBgmCrossfade() {
        const auto now = std::chrono::steady_clock::now();
        if (previousBgmUpdateTime_ ==
            std::chrono::steady_clock::time_point{}) {
            previousBgmUpdateTime_ = now;
            return;
        }
        const float deltaTime = (std::min)(
            std::chrono::duration<float>(
                now - previousBgmUpdateTime_).count(),
            1.0f / 15.0f);
        previousBgmUpdateTime_ = now;
        if (!isBgmCrossfading_) {
            return;
        }

        bgmCrossfadeElapsedTime_ = (std::min)(
            bgmCrossfadeElapsedTime_ + deltaTime,
            kBgmCrossfadeDuration);
        const float progress = std::clamp(
            bgmCrossfadeElapsedTime_ / kBgmCrossfadeDuration,
            0.0f,
            1.0f);
        const float fadeAngle =
            progress * std::numbers::pi_v<float> * 0.5f;
        if (outgoingBgmAudio_) {
            outgoingBgmAudio_->SetVolume(
                kBgmVolume * std::cos(fadeAngle));
        }
        if (bgmAudio_) {
            bgmAudio_->SetVolume(
                kBgmVolume * std::sin(fadeAngle));
        }

        if (progress >= 1.0f) {
            if (outgoingBgmAudio_) {
                outgoingBgmAudio_->Stop();
                outgoingBgmAudio_.reset();
            }
            if (bgmAudio_) {
                bgmAudio_->SetVolume(kBgmVolume);
            }
            isBgmCrossfading_ = false;
        }
    }

    void Engine::PlaySoundEffect(const std::string& filePath, float volume) {
        if (!audioManager_) {
            return;
        }
        soundEffects_.push_back(audioManager_->Play(filePath, false, volume));
    }

    void Engine::UpdateAudio() {
        soundEffects_.erase(
            std::remove_if(
                soundEffects_.begin(), soundEffects_.end(),
                [](const std::unique_ptr<Audio>& audio) {
                    return !audio || !audio->IsPlaying();
                }),
            soundEffects_.end());

        const SceneType currentScene = sceneManager_.GetCurrentScene();
        const int32_t currentBossPhase = gameScene_.GetBossPhase();
        const bool isNightmareMode = gameScene_.IsNightmareMode();

        if (currentScene == SceneType::Title &&
            isNightmareMode != wasNightmareModeEnabled_) {
            PlaySoundEffect(kNightmareSoundPath);
        }
        wasNightmareModeEnabled_ = isNightmareMode;

        if (currentScene != previousAudioScene_) {
            const bool isSelectTransition =
                (previousAudioScene_ == SceneType::Title &&
                    currentScene == SceneType::Tutorial) ||
                (previousAudioScene_ == SceneType::Tutorial &&
                    currentScene == SceneType::Game) ||
                ((previousAudioScene_ == SceneType::Clear ||
                    previousAudioScene_ == SceneType::Miss) &&
                    currentScene == SceneType::Title);
            if (isSelectTransition) {
                PlaySoundEffect(kSelectSoundPath);
            }
        }

        if (currentScene == SceneType::Title || currentScene == SceneType::Tutorial) {
            PlayBgm(kTitleBgmPath);
        }
        else if (currentScene == SceneType::Game) {
            const bool isPhase2BossDefeated =
                currentBossPhase == 2 &&
                !gameScene_.IsPhase2IntroActive() &&
                gameScene_.GetEnemyHealth(0) <= 0.0f;
            PlayBgm(
                isPhase2BossDefeated
                    ? ""
                    : (currentBossPhase == 2
                        ? kPhase2BgmPath
                        : kPhase1BgmPath));
        }
        else if (currentScene == SceneType::Clear) {
            PlayBgm(kGameClearBgmPath);
        }
        else if (currentScene == SceneType::Miss) {
            PlayBgm(kGameOverBgmPath);
        }
        else {
            PlayBgm("");
        }
        UpdateBgmCrossfade();

        const bool shouldPlayCharge =
            sceneManager_.IsPlayableScene() &&
            gameScene_.ShouldPlayPlayerChargeSound() &&
            gameScene_.GetRemainingProjectileCount() > 0;
        if (shouldPlayCharge && !chargeAudio_) {
            chargeAudio_ = audioManager_->Play(
                kChargeSoundPath,
                true,
                gameScene_.IsPlayerChargeComplete()
                    ? kCompletedChargeVolume
                    : kChargeVolume);
        }
        else if (shouldPlayCharge && chargeAudio_) {
            chargeAudio_->SetVolume(
                gameScene_.IsPlayerChargeComplete()
                    ? kCompletedChargeVolume
                    : kChargeVolume);
        }
        else if (!shouldPlayCharge && chargeAudio_) {
            chargeAudio_->Stop();
            chargeAudio_.reset();
        }

        const int32_t firedAmmo = gameScene_.ConsumeFiredProjectileAmmo();
        if (firedAmmo == 1) {
            PlaySoundEffect(kShotSoundPath, kShotVolume);
        }
        else if (firedAmmo > 1) {
            PlaySoundEffect(kChargeShotSoundPath, kChargeShotVolume);
        }
        if (gameScene_.ConsumeTeleportSoundRequest()) {
            PlaySoundEffect(kTeleportSoundPath, kEnemyMovementSoundVolume);
        }
        if (gameScene_.ConsumeDashSoundRequest()) {
            PlaySoundEffect(kDashSoundPath, kEnemyMovementSoundVolume);
        }
        if (gameScene_.ConsumeAlertSoundRequest()) {
            PlaySoundEffect(kAlertSoundPath, kEnemyMovementSoundVolume);
        }
        if (gameScene_.ConsumeFallSoundRequest()) {
            PlaySoundEffect(kFallSoundPath, kEnemyMovementSoundVolume);
        }
        if (gameScene_.ConsumeDamageSoundRequest()) {
            PlaySoundEffect(kDamageSoundPath, kDamageSoundVolume);
        }
        if (gameScene_.ConsumeEnemyDamageSoundRequest()) {
            PlaySoundEffect(kEnemyDamageSoundPath, kEnemyMovementSoundVolume);
        }

        previousAudioScene_ = currentScene;
    }

}
