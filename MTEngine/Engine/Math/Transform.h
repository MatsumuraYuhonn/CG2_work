#pragma once
#include "Vector.h"

namespace MTEngine {

    // トランスフォーム情報構造体
    struct Transform {
        Vector3 scale = { 1.0f,1.0f,1.0f };      // スケール
        Vector3 rotate = { 0.0f, 0.0f, 0.0f }; // 回転（オイラー角）
        Vector3 translate = { 0.0f, 0.0f, 0.0f };  // 平行移動
    };

}