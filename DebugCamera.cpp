#include "DebugCamera.h"

void DebugCamera::Initialize()
{

	translation_ = { 0.0f, 0.0f, -50.0f };
	rotation_ = { 0.0f, 0.0f, 0.0f };

	target_ = { 0.0f, 0.0f, 0.0f }; // 原点を中心に回るように設定
	distance_ = 50.0f;

	matRot_ = MakeIdentityMatrix();

}

// 毎フレームの更新処理
void DebugCamera::Update()
{

	POINT mousePos;
	GetCursorPos(&mousePos);
	Vector2 currentMousePos = { (float)mousePos.x, (float)mousePos.y };

	Vector2 mouseDelta = {
		currentMousePos.x - preMousePosition_.x,
		currentMousePos.y - preMousePosition_.y
	};


	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000 && key[DIK_LSHIFT]) {
		const float sensitivity = 0.001f;
		float deltaX = mouseDelta.y * sensitivity;
		float deltaY = mouseDelta.x * sensitivity;

		Matrix4x4 matRotDelta = MakeIdentityMatrix();
		matRotDelta *= MakeRotationXMatrix(deltaX);
		matRotDelta *= MakeRotationYMatrix(deltaY);

		matRot_ = matRotDelta * matRot_;

	}

	preMousePosition_ = currentMousePos;

	
	// カメラの移動(上下左右)
    if (key[DIK_W]) {
        const float speed = 0.1f;
        Vector3 move = { 0, speed, 0 };
        move = TransformNormal(move, matRot_);
        target_ = Add(target_, move);
    }
    if (key[DIK_S]) {
        const float speed = -0.1f;
        Vector3 move = { 0, speed, 0 };
        move = TransformNormal(move, matRot_);
        target_ = Add(target_, move);
    }
    if (key[DIK_D]) {
        const float speed = 0.1f;
        Vector3 move = { speed, 0, 0 };
        move = TransformNormal(move, matRot_);
        target_ = Add(target_, move);
    }
    if (key[DIK_A]) {
        const float speed = -0.1f;
        Vector3 move = { speed, 0, 0 };
        move = TransformNormal(move, matRot_);
        target_ = Add(target_, move);
    }

    if (key[DIK_UP]) {
        const float speed = 0.5f;
        distance_ -= speed; // 近づく
        if (distance_ < 1.0f) distance_ = 1.0f; // めり込み防止
    }
    if (key[DIK_DOWN]) {
        const float speed = 0.5f;
        distance_ += speed; // 遠ざかる
    }

    // --- 行列の計算 ---
    Matrix4x4 matScale = MakeScaleMatrix(cameraTransform.scale);

    Matrix4x4 matOffset = MakeTranslateMatrix({ 0.0f, 0.0f, -distance_ });

    Matrix4x4 matTargetTrans = MakeTranslateMatrix(target_);

    cameraMatrix = matScale * matOffset * matRot_ * matTargetTrans;

    viewMatrix = Inverse(cameraMatrix);
    projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
}