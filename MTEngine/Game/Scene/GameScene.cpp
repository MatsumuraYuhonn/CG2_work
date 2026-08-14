#include "GameScene.h"
#include "MTEngine/Engine/Base/Window.h"
#include "MTEngine/Engine/Input/Input.h"

#include <charconv>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <fstream>
#include <numbers>
#include <sstream>

namespace MTEngine {

    namespace {

        Matrix4x4 MakeOrientationMarkerProjectionMatrix() {
            Matrix4x4 projection{};
            projection.m[0][0] = 0.12f;
            projection.m[1][1] = 0.12f;
            projection.m[2][2] = 0.01f;
            projection.m[3][2] = 0.10f;
            projection.m[3][3] = 1.0f;
            return projection;
        }

        bool TryParseTileId(const std::string& text, int32_t& tileId) {
            const size_t first = text.find_first_not_of(" \t");
            if (first == std::string::npos) {
                return false;
            }

            const size_t last = text.find_last_not_of(" \t");
            const char* begin = text.data() + first;
            const char* end = text.data() + last + 1;
            const auto [parsedEnd, error] = std::from_chars(begin, end, tileId);
            return error == std::errc{} && parsedEnd == end;
        }

    }

    bool TileMap::LoadFromCsv(const std::string& filePath) {
        Clear();

        std::ifstream file(filePath);
        if (!file.is_open()) {
            lastError_ = "CSV file could not be opened: " + filePath;
            return false;
        }

        std::string line;
        int32_t csvLineNumber = 0;
        while (std::getline(file, line)) {
            ++csvLineNumber;

            // UTF-8 BOM が先頭に存在しても、最初の数値を正しく読み込めるようにします。
            if (csvLineNumber == 1 && line.starts_with("\xEF\xBB\xBF")) {
                line.erase(0, 3);
            }

            const size_t first = line.find_first_not_of(" \t\r");
            if (first == std::string::npos || line[first] == '#') {
                continue;
            }

            std::vector<int32_t> row;
            std::stringstream stream(line);
            std::string field;
            while (std::getline(stream, field, ',')) {
                int32_t tileId = kEmptyTileId;
                if (!TryParseTileId(field, tileId)) {
                    Clear();
                    lastError_ = "Invalid tile ID at CSV line " + std::to_string(csvLineNumber) + ".";
                    return false;
                }
                row.push_back(tileId);
            }

            if (row.empty()) {
                Clear();
                lastError_ = "Empty row at CSV line " + std::to_string(csvLineNumber) + ".";
                return false;
            }

            if (width_ == 0) {
                width_ = static_cast<int32_t>(row.size());
            }
            else if (static_cast<int32_t>(row.size()) != width_) {
                Clear();
                lastError_ = "Column count differs at CSV line " + std::to_string(csvLineNumber) + ".";
                return false;
            }

            tiles_.insert(tiles_.end(), row.begin(), row.end());
            ++height_;
        }

        if (tiles_.empty()) {
            lastError_ = "CSV contains no tile rows: " + filePath;
            return false;
        }

        return true;
    }

    void TileMap::Clear() {
        width_ = 0;
        height_ = 0;
        tiles_.clear();
        lastError_.clear();
    }

    int32_t TileMap::GetTileId(int32_t column, int32_t row) const {
        if (column < 0 || row < 0 || column >= width_ || row >= height_) {
            return kEmptyTileId;
        }
        return tiles_[static_cast<size_t>(row) * width_ + column];
    }

    std::vector<TileMapCell> TileMap::CreateCells(float tileSize, const Vector3& origin) const {
        std::vector<TileMapCell> cells;
        if (tileSize <= 0.0f) {
            return cells;
        }

        cells.reserve(tiles_.size());
        for (int32_t row = 0; row < height_; ++row) {
            for (int32_t column = 0; column < width_; ++column) {
                const int32_t tileId = GetTileId(column, row);
                if (tileId == kEmptyTileId) {
                    continue;
                }

                cells.push_back({
                    tileId,
                    column,
                    row,
                    {
                        origin.x + static_cast<float>(column) * tileSize,
                        origin.y + static_cast<float>(height_ - 1 - row) * tileSize,
                        origin.z,
                    },
                });
            }
        }
        return cells;
    }

    bool GameScene::Initialize(
        Microsoft::WRL::ComPtr<ID3D12Device> device,
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
        DescriptorHeapManager* srvHeapManager,
        const std::string& csvFilePath)
    {
        if (!tileMap_.LoadFromCsv(csvFilePath)) {
            return false;
        }

        constexpr float kTileSize = 2.0f;
        const Vector3 mapOrigin = {
            -static_cast<float>(tileMap_.GetWidth() - 1) * kTileSize * 0.5f,
            -static_cast<float>(tileMap_.GetHeight() - 1) * kTileSize * 0.5f,
            0.0f,
        };
        cells_ = tileMap_.CreateCells(kTileSize, mapOrigin);

        const ModelData cubeModelData = Model::LoadObjFile("MTEngine/Assets/Resources/cube", "cube.obj");
        if (!cubeModel_.Initialize(device, cubeModelData)) {
            return false;
        }

        const ModelData playerModelData = Model::LoadObjFile("MTEngine/Assets/Resources/player", "player.obj");
        if (!playerModel_.Initialize(device, playerModelData)) {
            return false;
        }

        textureManager_.Initialize(device, srvHeapManager);
        for (const MeshData& mesh : cubeModelData.meshes) {
            if (!mesh.material.textureFilePath.empty()) {
                textureManager_.Load(mesh.material.textureFilePath, commandList);
            }
        }
        for (const MeshData& mesh : playerModelData.meshes) {
            if (!mesh.material.textureFilePath.empty()) {
                textureManager_.Load(mesh.material.textureFilePath, commandList);
            }
        }

        if (!materialConstantBuffer_.Initialize(device) ||
            !playerMaterialConstantBuffer_.Initialize(device) ||
            !playerTransformConstantBuffer_.Initialize(device) ||
            !selectionMaterialConstantBuffer_.Initialize(device) ||
            !lightConstantBuffer_.Initialize(device)) {
            return false;
        }

        transformConstantBuffers_.clear();
        transformConstantBuffers_.reserve(cells_.size());
        for (size_t cellIndex = 0; cellIndex < cells_.size(); ++cellIndex) {
            auto transformConstantBuffer = std::make_unique<TransformationMatrixConstantBuffer>();
            if (!transformConstantBuffer->Initialize(device)) {
                return false;
            }
            transformConstantBuffers_.push_back(std::move(transformConstantBuffer));
        }

        for (size_t axisIndex = 0; axisIndex < axisMaterialConstantBuffers_.size(); ++axisIndex) {
            if (!axisMaterialConstantBuffers_[axisIndex].Initialize(device) ||
                !axisTransformConstantBuffers_[axisIndex].Initialize(device)) {
                return false;
            }
        }

        materialConstantBuffer_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        materialConstantBuffer_->enabledLighting = 0;
        materialConstantBuffer_->uvTransform = MakeIdentityMatrix();
        materialConstantBuffer_->lightingMode = 2;
        materialConstantBuffer_->useTexture = 1;
        materialConstantBuffer_->isSelected = 0;

        playerMaterialConstantBuffer_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        playerMaterialConstantBuffer_->enabledLighting = 1;
        playerMaterialConstantBuffer_->uvTransform = MakeIdentityMatrix();
        playerMaterialConstantBuffer_->lightingMode = 2;
        playerMaterialConstantBuffer_->useTexture = 1;
        playerMaterialConstantBuffer_->isSelected = 0;

        selectionMaterialConstantBuffer_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        selectionMaterialConstantBuffer_->enabledLighting = 0;
        selectionMaterialConstantBuffer_->uvTransform = MakeIdentityMatrix();
        selectionMaterialConstantBuffer_->lightingMode = 0;
        selectionMaterialConstantBuffer_->useTexture = 1;
        selectionMaterialConstantBuffer_->isSelected = 1;

        const Vector4 axisColors[] = {
            { 0.95f, 0.15f, 0.15f, 1.0f }, // +X
            { 0.15f, 0.95f, 0.25f, 1.0f }, // +Y
            { 0.20f, 0.45f, 1.00f, 1.0f }, // +Z
        };
        for (size_t axisIndex = 0; axisIndex < axisMaterialConstantBuffers_.size(); ++axisIndex) {
            MaterialConstantBuffer& axisMaterial = axisMaterialConstantBuffers_[axisIndex];
            axisMaterial->color = axisColors[axisIndex];
            axisMaterial->enabledLighting = 0;
            axisMaterial->uvTransform = MakeIdentityMatrix();
            axisMaterial->lightingMode = 0;
            axisMaterial->useTexture = 0;
            axisMaterial->isSelected = 0;
        }

        lightConstantBuffer_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        lightConstantBuffer_->direction = { 0.3f, -1.0f, 0.2f };
        lightConstantBuffer_->intensity = 1.0f;

        // The player model's bottom is 0.8 units below its origin at this scale.
        // Start on the floor inside the map.
        playerPosition_ = { -24.69f, -7.20f, -1.20f };
        playerRotationY_ = -std::numbers::pi_v<float> / 2.0f;
        playerHorizontalVelocity_ = 0.0f;
        playerVerticalVelocity_ = 0.0f;
        isPlayerGrounded_ = true;
        previousPlayerUpdateTime_ = std::chrono::steady_clock::now();
        return true;
    }

    void GameScene::UpdatePlayer(const Input* input) {
        if (previousPlayerUpdateTime_ == std::chrono::steady_clock::time_point{}) {
            previousPlayerUpdateTime_ = std::chrono::steady_clock::now();
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        const float deltaTime = (std::min)(
            std::chrono::duration<float>(now - previousPlayerUpdateTime_).count(),
            1.0f / 30.0f);
        previousPlayerUpdateTime_ = now;

        constexpr float kGravity = -34.0f;
        constexpr float kJumpSpeed = 16.5f;
        constexpr float kPlayerHalfWidth = 0.6f;
        constexpr float kPlayerHalfHeight = 0.8f;
        constexpr float kTileHalfExtent = 1.0f;
        constexpr float kTileTopOffset = 1.0f;
        constexpr float kPlayerTurnResponse = 12.0f;

        float horizontalInput = 0.0f;
        bool jumpRequested = false;
        if (input) {
            if (input->PushKey(DIK_A) || input->PushKey(DIK_LEFT)) {
                horizontalInput -= 1.0f;
            }
            if (input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT)) {
                horizontalInput += 1.0f;
            }
            jumpRequested = input->TriggerKey(DIK_W);

            const GamePad* gamePad = input->GetGamePad();
            if (gamePad && gamePad->IsConnected()) {
                horizontalInput += gamePad->GetLeftStick().x;
                jumpRequested = jumpRequested || gamePad->TriggerButton(GamePadButton::B);
            }

            horizontalInput = std::clamp(horizontalInput, -1.0f, 1.0f);
            if (isPlayerGrounded_ && jumpRequested) {
                playerVerticalVelocity_ = kJumpSpeed;
                isPlayerGrounded_ = false;
            }
        }

        if (horizontalInput != 0.0f) {
            playerHorizontalVelocity_ += horizontalInput * playerMoveAcceleration_ * deltaTime;
        }
        else {
            playerHorizontalVelocity_ *= std::exp(-playerVelocityDamping_ * deltaTime);
            if (std::abs(playerHorizontalVelocity_) < 0.01f) {
                playerHorizontalVelocity_ = 0.0f;
            }
        }
        playerHorizontalVelocity_ = std::clamp(
            playerHorizontalVelocity_,
            -playerMaxMoveSpeed_,
            playerMaxMoveSpeed_);

        // Keep facing the actual movement direction, but turn toward it smoothly.
        if (std::abs(playerHorizontalVelocity_) >= 0.01f) {
            const float targetRotationY = playerHorizontalVelocity_ > 0.0f
                ? -std::numbers::pi_v<float> / 2.0f
                : std::numbers::pi_v<float> / 2.0f;
            const float rotationDifference = std::remainder(
                targetRotationY - playerRotationY_,
                2.0f * std::numbers::pi_v<float>);
            const float interpolation = 1.0f - std::exp(-kPlayerTurnResponse * deltaTime);
            playerRotationY_ = std::remainder(
                playerRotationY_ + rotationDifference * interpolation,
                2.0f * std::numbers::pi_v<float>);
        }

        const float previousX = playerPosition_.x;
        playerPosition_.x += playerHorizontalVelocity_ * deltaTime;

        const float playerBottom = playerPosition_.y - kPlayerHalfHeight;
        const float playerTop = playerPosition_.y + kPlayerHalfHeight;
        for (const TileMapCell& cell : cells_) {
            const float tileBottom = cell.worldPosition.y - kTileHalfExtent;
            const float tileTop = cell.worldPosition.y + kTileHalfExtent;
            if (playerBottom >= tileTop || playerTop <= tileBottom) {
                continue;
            }

            const float tileLeft = cell.worldPosition.x - kTileHalfExtent;
            const float tileRight = cell.worldPosition.x + kTileHalfExtent;
            const float previousLeft = previousX - kPlayerHalfWidth;
            const float previousRight = previousX + kPlayerHalfWidth;
            const float playerLeft = playerPosition_.x - kPlayerHalfWidth;
            const float playerRight = playerPosition_.x + kPlayerHalfWidth;

            if (playerHorizontalVelocity_ < 0.0f && previousLeft >= tileRight && playerLeft < tileRight) {
                playerPosition_.x = (std::max)(playerPosition_.x, tileRight + kPlayerHalfWidth);
                playerHorizontalVelocity_ = 0.0f;
            }
            else if (playerHorizontalVelocity_ > 0.0f && previousRight <= tileLeft && playerRight > tileLeft) {
                playerPosition_.x = (std::min)(playerPosition_.x, tileLeft - kPlayerHalfWidth);
                playerHorizontalVelocity_ = 0.0f;
            }
        }

        playerVerticalVelocity_ += kGravity * deltaTime;
        const float previousY = playerPosition_.y;
        playerPosition_.y += playerVerticalVelocity_ * deltaTime;

        isPlayerGrounded_ = false;
        float landingY = -FLT_MAX;
        for (const TileMapCell& cell : cells_) {
            if (std::abs(playerPosition_.x - cell.worldPosition.x) > kTileHalfExtent + kPlayerHalfWidth) {
                continue;
            }

            const float tileTop = cell.worldPosition.y + kTileTopOffset;
            const float playerBottomBeforeMove = previousY - kPlayerHalfHeight;
            const float playerBottomAfterMove = playerPosition_.y - kPlayerHalfHeight;
            if (playerVerticalVelocity_ <= 0.0f &&
                playerBottomBeforeMove >= tileTop &&
                playerBottomAfterMove <= tileTop) {
                landingY = (std::max)(landingY, tileTop + kPlayerHalfHeight);
            }
        }

        if (landingY > -FLT_MAX) {
            playerPosition_.y = landingY;
            playerVerticalVelocity_ = 0.0f;
            isPlayerGrounded_ = true;
        }
    }

    void GameScene::Draw(
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
        const Matrix4x4& viewMatrix,
        const Matrix4x4& projectionMatrix)
    {
        commandList->SetGraphicsRootConstantBufferView(0, materialConstantBuffer_.GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(2, lightConstantBuffer_.GetGPUVirtualAddress());

        constexpr Vector3 kCubeScale = { 1.0f, 1.0f, 1.0f };
        for (size_t cellIndex = 0; cellIndex < cells_.size(); ++cellIndex) {
            const TileMapCell& cell = cells_[cellIndex];
            TransformationMatrixConstantBuffer& transformConstantBuffer = *transformConstantBuffers_[cellIndex];
            const Matrix4x4 worldMatrix = MakeAffineMatrix(
                kCubeScale,
                { 0.0f, 0.0f, 0.0f },
                cell.worldPosition);
            transformConstantBuffer->World = worldMatrix;
            transformConstantBuffer->WVP = Multiply(Multiply(worldMatrix, viewMatrix), projectionMatrix);
            commandList->SetGraphicsRootConstantBufferView(
                0,
                cellIndex == static_cast<size_t>(selectedCellIndex_)
                    ? selectionMaterialConstantBuffer_.GetGPUVirtualAddress()
                    : materialConstantBuffer_.GetGPUVirtualAddress());
            commandList->SetGraphicsRootConstantBufferView(1, transformConstantBuffer.GetGPUVirtualAddress());
            cubeModel_.Draw(commandList, &textureManager_);
        }

        constexpr Vector3 kPlayerScale = { 2.0f, 2.0f, 2.0f };
        const Vector3 playerRotation = { 0.0f, playerRotationY_, 0.0f };
        const Matrix4x4 playerWorldMatrix = MakeAffineMatrix(
            kPlayerScale,
            playerRotation,
            playerPosition_);
        playerTransformConstantBuffer_->World = playerWorldMatrix;
        playerTransformConstantBuffer_->WVP = Multiply(
            Multiply(playerWorldMatrix, viewMatrix), projectionMatrix);
        commandList->SetGraphicsRootConstantBufferView(
            0,
            playerMaterialConstantBuffer_.GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(
            1,
            playerTransformConstantBuffer_.GetGPUVirtualAddress());
        playerModel_.Draw(commandList, &textureManager_);

        constexpr int32_t kMarkerSize = 128;
        constexpr int32_t kMarkerMargin = 16;
        D3D12_VIEWPORT markerViewport{};
        markerViewport.TopLeftX = static_cast<float>(kClientWidth - kMarkerMargin - kMarkerSize);
        markerViewport.TopLeftY = static_cast<float>(kMarkerMargin);
        markerViewport.Width = static_cast<float>(kMarkerSize);
        markerViewport.Height = static_cast<float>(kMarkerSize);
        markerViewport.MinDepth = 0.0f;
        markerViewport.MaxDepth = 1.0f;
        const D3D12_RECT markerScissorRect{
            static_cast<LONG>(markerViewport.TopLeftX),
            static_cast<LONG>(markerViewport.TopLeftY),
            static_cast<LONG>(markerViewport.TopLeftX + markerViewport.Width),
            static_cast<LONG>(markerViewport.TopLeftY + markerViewport.Height),
        };
        commandList->RSSetViewports(1, &markerViewport);
        commandList->RSSetScissorRects(1, &markerScissorRect);

        Matrix4x4 rotationOnlyView = viewMatrix;
        rotationOnlyView.m[3][0] = 0.0f;
        rotationOnlyView.m[3][1] = 0.0f;
        rotationOnlyView.m[3][2] = 0.0f;
        rotationOnlyView.m[3][3] = 1.0f;

        // Positive world axes: X is red, Y is green, and Z is blue.
        constexpr Vector3 kAxisScales[] = {
            { 3.0f, 0.08f, 0.08f },
            { 0.08f, 3.0f, 0.08f },
            { 0.08f, 0.08f, 3.0f },
        };
        constexpr Vector3 kAxisPositions[] = {
            { 3.0f, 0.0f, 0.0f },
            { 0.0f, 3.0f, 0.0f },
            { 0.0f, 0.0f, 3.0f },
        };
        for (size_t axisIndex = 0; axisIndex < axisTransformConstantBuffers_.size(); ++axisIndex) {
            TransformationMatrixConstantBuffer& axisTransform = axisTransformConstantBuffers_[axisIndex];
            const Matrix4x4 worldMatrix = MakeAffineMatrix(
                kAxisScales[axisIndex],
                { 0.0f, 0.0f, 0.0f },
                kAxisPositions[axisIndex]);
            axisTransform->World = worldMatrix;
            axisTransform->WVP = Multiply(
                Multiply(worldMatrix, rotationOnlyView),
                MakeOrientationMarkerProjectionMatrix());
            commandList->SetGraphicsRootConstantBufferView(
                0,
                axisMaterialConstantBuffers_[axisIndex].GetGPUVirtualAddress());
            commandList->SetGraphicsRootConstantBufferView(1, axisTransform.GetGPUVirtualAddress());
            cubeModel_.Draw(commandList, &textureManager_);
        }

        D3D12_VIEWPORT sceneViewport{};
        sceneViewport.Width = static_cast<float>(kClientWidth);
        sceneViewport.Height = static_cast<float>(kClientHeight);
        sceneViewport.MinDepth = 0.0f;
        sceneViewport.MaxDepth = 1.0f;
        const D3D12_RECT sceneScissorRect{ 0, 0, kClientWidth, kClientHeight };
        commandList->RSSetViewports(1, &sceneViewport);
        commandList->RSSetScissorRects(1, &sceneScissorRect);
    }

}
