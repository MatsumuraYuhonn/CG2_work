#pragma once

namespace MTEngine {

    // 4次元ベクトル
    struct Vector4 {
        float x, y, z, w;
    };

    // 3次元ベクトル
    struct Vector3 {
        float x, y, z;
    };

    // 2次元ベクトル
    struct Vector2 {
        float x, y;
    };

    // Matrix4x4の前方宣言
    struct Matrix4x4;

    // ベクトルの加算
    Vector3 Add(const Vector3& v1, const Vector3& v2);

    // 法線ベクトルの変換
    Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);

}