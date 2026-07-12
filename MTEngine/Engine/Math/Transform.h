#pragma once
#include "Vector.h"

// トランスフォーム情報構造体
struct Transform {
    Vector3 scale;      // スケール
    Vector3 rotate;     // 回転（オイラー角）
    Vector3 translate;  // 平行移動
};