#pragma once
#include <cmath>
#include "Vector.h"

// -------------------------
// 構造体
// -------------------------

namespace MTEngine {

    struct Matrix3x3 {
        float m[3][3];
    };

    struct Matrix4x4 {
        float m[4][4];
    };

    // 描画用パイプラインで利用する行列セット
    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
    };

}

// -------------------------
// 関数
// -------------------------

namespace MTEngine {

    // 行列の掛け算
    inline Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
        Matrix4x4 result = {};
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i][j] = m1.m[i][0] * m2.m[0][j] +
                    m1.m[i][1] * m2.m[1][j] +
                    m1.m[i][2] * m2.m[2][j] +
                    m1.m[i][3] * m2.m[3][j];
            }
        }
        return result;
    }

    // 単位行列の生成
    inline Matrix4x4 MakeIdentityMatrix() {
        Matrix4x4 result = {};
        result.m[0][0] = 1.0f;
        result.m[1][1] = 1.0f;
        result.m[2][2] = 1.0f;
        result.m[3][3] = 1.0f;
        return result;
    }

    // スケーリング行列の生成
    inline Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
        Matrix4x4 result = MakeIdentityMatrix();
        result.m[0][0] = scale.x;
        result.m[1][1] = scale.y;
        result.m[2][2] = scale.z;
        return result;
    }

    // 平行移動行列の生成
    inline Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
        Matrix4x4 result = MakeIdentityMatrix();
        result.m[3][0] = translate.x;
        result.m[3][1] = translate.y;
        result.m[3][2] = translate.z;
        result.m[3][3] = 1.0f;
        return result;
    }

    // X軸回転行列の生成
    inline Matrix4x4 MakeRotationXMatrix(float radian) {
        Matrix4x4 result = MakeIdentityMatrix();
        float cosA = std::cos(radian);
        float sinA = std::sin(radian);
        result.m[1][1] = cosA;
        result.m[1][2] = sinA;
        result.m[2][1] = -sinA;
        result.m[2][2] = cosA;
        return result;
    }

    // Y軸回転行列の生成
    inline Matrix4x4 MakeRotationYMatrix(float radian) {
        Matrix4x4 result = MakeIdentityMatrix();
        float cosA = std::cos(radian);
        float sinA = std::sin(radian);
        result.m[0][0] = cosA;
        result.m[0][2] = -sinA;
        result.m[2][0] = sinA;
        result.m[2][2] = cosA;
        return result;
    }

    // Z軸回転行列の生成
    inline Matrix4x4 MakeRotationZMatrix(float radian) {
        Matrix4x4 result = MakeIdentityMatrix();
        float cosA = std::cos(radian);
        float sinA = std::sin(radian);
        result.m[0][0] = cosA;
        result.m[0][1] = sinA;
        result.m[1][0] = -sinA;
        result.m[1][1] = cosA;
        return result;
    }

    // XYZ合成回転行列の生成
    inline Matrix4x4 MakeRotateXYZMatrix(const Vector3& rot) {
        Matrix4x4 rotX = MakeRotationXMatrix(rot.x);
        Matrix4x4 rotY = MakeRotationYMatrix(rot.y);
        Matrix4x4 rotZ = MakeRotationZMatrix(rot.z);
        return Multiply(rotX, Multiply(rotY, rotZ));
    }

    // アフィン変換行列の生成
    inline Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rot, const Vector3& translate) {
        Matrix4x4 result = {};
        float sinX = std::sin(rot.x);
        float cosX = std::cos(rot.x);
        float sinY = std::sin(rot.y);
        float cosY = std::cos(rot.y);
        float sinZ = std::sin(rot.z);
        float cosZ = std::cos(rot.z);

        result.m[0][0] = scale.x * (cosY * cosZ + sinX * sinY * sinZ);
        result.m[0][1] = scale.x * (sinX * sinY * cosZ - cosY * sinZ);
        result.m[0][2] = scale.x * (cosX * sinY);
        result.m[0][3] = 0.0f;

        result.m[1][0] = scale.y * (cosX * sinZ);
        result.m[1][1] = scale.y * (cosX * cosZ);
        result.m[1][2] = scale.y * (-sinX);
        result.m[1][3] = 0.0f;

        result.m[2][0] = scale.z * (sinX * cosY * sinZ - sinY * cosZ);
        result.m[2][1] = scale.z * (sinX * cosY * cosZ + sinY * sinZ);
        result.m[2][2] = scale.z * (cosX * cosY);
        result.m[2][3] = 0.0f;

        result.m[3][0] = translate.x;
        result.m[3][1] = translate.y;
        result.m[3][2] = translate.z;
        result.m[3][3] = 1.0f;

        return result;
    }

    // 透視投影行列の生成
    inline Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
        Matrix4x4 result = {};
        float cot = 1.0f / std::tan(fovY / 2.0f);

        result.m[0][0] = cot / aspectRatio;
        result.m[1][1] = cot;
        result.m[2][2] = farClip / (farClip - nearClip);
        result.m[2][3] = 1.0f;
        result.m[3][2] = -(nearClip * farClip) / (farClip - nearClip);
        result.m[3][3] = 0.0f;

        return result;
    }

    // 逆行列の計算
    inline Matrix4x4 Inverse(const Matrix4x4& m) {
        Matrix4x4 result = {};

        float b00 = m.m[0][0] * m.m[1][1] - m.m[0][1] * m.m[1][0];
        float b01 = m.m[0][0] * m.m[1][2] - m.m[0][2] * m.m[1][0];
        float b02 = m.m[0][0] * m.m[1][3] - m.m[0][3] * m.m[1][0];
        float b03 = m.m[0][1] * m.m[1][2] - m.m[0][2] * m.m[1][1];
        float b04 = m.m[0][1] * m.m[1][3] - m.m[0][3] * m.m[1][1];
        float b05 = m.m[0][2] * m.m[1][3] - m.m[0][3] * m.m[1][2];
        float b06 = m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0];
        float b07 = m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0];
        float b08 = m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0];
        float b09 = m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1];
        float b10 = m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1];
        float b11 = m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2];

        float det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
        if (det == 0.0f) return result;

        float invDet = 1.0f / det;

        result.m[0][0] = (m.m[1][1] * b11 - m.m[1][2] * b10 + m.m[1][3] * b09) * invDet;
        result.m[0][1] = (-m.m[0][1] * b11 + m.m[0][2] * b10 - m.m[0][3] * b09) * invDet;
        result.m[0][2] = (m.m[3][1] * b05 - m.m[3][2] * b04 + m.m[3][3] * b03) * invDet;
        result.m[0][3] = (-m.m[2][1] * b05 + m.m[2][2] * b04 - m.m[2][3] * b03) * invDet;

        result.m[1][0] = (-m.m[1][0] * b11 + m.m[1][2] * b08 - m.m[1][3] * b07) * invDet;
        result.m[1][1] = (m.m[0][0] * b11 - m.m[0][2] * b08 + m.m[0][3] * b07) * invDet;
        result.m[1][2] = (-m.m[3][0] * b05 + m.m[3][2] * b02 - m.m[3][3] * b01) * invDet;
        result.m[1][3] = (m.m[2][0] * b05 - m.m[2][2] * b02 + m.m[2][3] * b01) * invDet;

        result.m[2][0] = (m.m[1][0] * b10 - m.m[1][1] * b08 + m.m[1][3] * b06) * invDet;
        result.m[2][1] = (-m.m[0][0] * b10 + m.m[0][1] * b08 - m.m[0][3] * b06) * invDet;
        result.m[2][2] = (m.m[3][0] * b04 - m.m[3][1] * b02 + m.m[3][3] * b00) * invDet;
        result.m[2][3] = (-m.m[2][0] * b04 + m.m[2][1] * b02 - m.m[2][3] * b00) * invDet;

        result.m[3][0] = (-m.m[1][0] * b09 + m.m[1][1] * b07 - m.m[1][2] * b06) * invDet;
        result.m[3][1] = (m.m[0][0] * b09 - m.m[0][1] * b07 + m.m[0][2] * b06) * invDet;
        result.m[3][2] = (-m.m[3][0] * b03 + m.m[3][1] * b01 - m.m[3][2] * b00) * invDet;
        result.m[3][3] = (m.m[2][0] * b03 - m.m[2][1] * b01 + m.m[2][2] * b00) * invDet;

        return result;
    }

    // 演算子のオーバーロード
    inline Matrix4x4& operator*=(Matrix4x4& m1, const Matrix4x4& m2) {
        m1 = Multiply(m1, m2);
        return m1;
    }

    inline Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2) {
        return Multiply(m1, m2);
    }

}