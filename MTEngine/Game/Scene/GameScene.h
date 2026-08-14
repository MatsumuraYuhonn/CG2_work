#pragma once

#include <d3d12.h>

#include "MTEngine/Engine/Graphics/ConstantBuffer.h"
#include "MTEngine/Engine/Graphics/Model.h"
#include "MTEngine/Engine/Graphics/TextureManager.h"
#include "MTEngine/Engine/Math/Matrix.h"
#include "MTEngine/Engine/Math/Vector.h"

#include <cstdint>
#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace MTEngine {

    class Input;

    // CSV で指定された 1 マス分のマップチップ情報です。
    struct TileMapCell {
        int32_t tileId = -1;
        int32_t column = 0;
        int32_t row = 0;
        Vector3 worldPosition{};
    };

    // 整数 CSV を読み込み、マップチップを配置するためのステージデータを保持します。
    //
    // CSV 例 ( 0 は空マス ):
    //  1,1,2,2
    //  1,0,0,2
    //  3,3,3,3
    class TileMap {
    public:
        static constexpr int32_t kEmptyTileId = 0;

        // CSV を読み込みます。失敗時は false を返し、GetLastError() に理由を設定します。
        bool LoadFromCsv(const std::string& filePath);

        void Clear();

        int32_t GetWidth() const { return width_; }
        int32_t GetHeight() const { return height_; }
        bool IsEmpty() const { return tiles_.empty(); }
        const std::string& GetLastError() const { return lastError_; }

        // 範囲外の場合は kEmptyTileId を返します。
        int32_t GetTileId(int32_t column, int32_t row) const;

        // 空マスを除いた配置情報を作成します。
        // column は +X、row は -Z 方向に並びます。
        std::vector<TileMapCell> CreateCells(float tileSize, const Vector3& origin = {}) const;

    private:
        int32_t width_ = 0;
        int32_t height_ = 0;
        std::vector<int32_t> tiles_;
        std::string lastError_;
    };

    // Loads the cube resource once and draws one cube for every non-empty CSV cell.
    // Owns the gameplay objects in this scene: the tile map and player.
    class GameScene {
    public:
        bool Initialize(
            Microsoft::WRL::ComPtr<ID3D12Device> device,
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
            DescriptorHeapManager* srvHeapManager,
            const std::string& csvFilePath);

        void Draw(
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
            const Matrix4x4& viewMatrix,
            const Matrix4x4& projectionMatrix);

        // Advances player movement and physics against non-empty map tiles.
        void UpdatePlayer(const Input* input);

        void SetSelectedCellIndex(int32_t selectedCellIndex) { selectedCellIndex_ = selectedCellIndex; }

        const TileMap& GetTileMap() const { return tileMap_; }
        const std::vector<TileMapCell>& GetCells() const { return cells_; }
        const Vector3& GetPlayerPosition() const { return playerPosition_; }
        float GetPlayerVerticalVelocity() const { return playerVerticalVelocity_; }
        float GetPlayerHorizontalVelocity() const { return playerHorizontalVelocity_; }
        float GetPlayerMoveAcceleration() const { return playerMoveAcceleration_; }
        float GetPlayerVelocityDamping() const { return playerVelocityDamping_; }
        float GetPlayerMaxMoveSpeed() const { return playerMaxMoveSpeed_; }
        bool IsPlayerGrounded() const { return isPlayerGrounded_; }
        void SetPlayerPosition(const Vector3& position) {
            playerPosition_ = position;
            playerHorizontalVelocity_ = 0.0f;
            playerVerticalVelocity_ = 0.0f;
            isPlayerGrounded_ = false;
        }
        void SetPlayerMoveAcceleration(float acceleration) { playerMoveAcceleration_ = acceleration < 0.0f ? 0.0f : acceleration; }
        void SetPlayerVelocityDamping(float damping) { playerVelocityDamping_ = damping < 0.0f ? 0.0f : damping; }
        void SetPlayerMaxMoveSpeed(float maxMoveSpeed) {
            playerMaxMoveSpeed_ = maxMoveSpeed < 0.0f ? 0.0f : maxMoveSpeed;
            if (playerHorizontalVelocity_ > playerMaxMoveSpeed_) {
                playerHorizontalVelocity_ = playerMaxMoveSpeed_;
            }
            else if (playerHorizontalVelocity_ < -playerMaxMoveSpeed_) {
                playerHorizontalVelocity_ = -playerMaxMoveSpeed_;
            }
        }

    private:
        TileMap tileMap_;
        std::vector<TileMapCell> cells_;
        Model cubeModel_;
        Model playerModel_;
        TextureManager textureManager_;
        MaterialConstantBuffer materialConstantBuffer_;
        MaterialConstantBuffer playerMaterialConstantBuffer_;
        std::vector<std::unique_ptr<TransformationMatrixConstantBuffer>> transformConstantBuffers_;
        TransformationMatrixConstantBuffer playerTransformConstantBuffer_;
        MaterialConstantBuffer selectionMaterialConstantBuffer_;
        std::array<MaterialConstantBuffer, 3> axisMaterialConstantBuffers_;
        std::array<TransformationMatrixConstantBuffer, 3> axisTransformConstantBuffers_;
        DirectionalLightConstantBuffer lightConstantBuffer_;
        Vector3 playerPosition_{};
        float playerRotationY_ = 0.0f;
        float playerHorizontalVelocity_ = 0.0f;
        float playerVerticalVelocity_ = 0.0f;
        float playerMoveAcceleration_ = 50.0f;
        float playerVelocityDamping_ = 10.0f;
        float playerMaxMoveSpeed_ = 15.0f;
        bool isPlayerGrounded_ = false;
        std::chrono::steady_clock::time_point previousPlayerUpdateTime_{};
        int32_t selectedCellIndex_ = -1;
    };

}
