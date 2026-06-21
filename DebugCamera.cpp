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

    // --- 前後移動（注視点との距離 distance_ を増減させる） ---
    // もしくはマウスホイール等に割り当てるとより直感的になります
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

    // 1. 最初に行列をカメラの初期位置（注視点から Z軸方向にマイナス（後ろ）に離れた位置）にする
    Matrix4x4 matOffset = MakeTranslateMatrix({ 0.0f, 0.0f, -distance_ });

    // 2. 注視点への平行移動行列
    Matrix4x4 matTargetTrans = MakeTranslateMatrix(target_);

    // 3. 行列を合成する
    // 【順序】スケール -> 距離分下がる -> 注視点中心に回転する -> 注視点の位置へ移動する
    cameraMatrix = matScale * matOffset * matRot_ * matTargetTrans;

    // ビュー行列・プロジェクション行列の計算（ここはそのまま）
    viewMatrix = Inverse(cameraMatrix);
    projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
}