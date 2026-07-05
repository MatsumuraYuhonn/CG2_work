#pragma once
#include <Windows.h>
#include "Vector.h"
#include "Matrix.h"
#include "Window.h"
#include "Transform.h"
#include "Input.h"


class DebugCamera {

public:

	void Initialize();

	void Update(const Input* input);


	const Matrix4x4& GetViewMatrix() const { return viewMatrix; }
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix; }


private:

	Matrix4x4 cameraMatrix;

	Matrix4x4 viewMatrix{};

	Matrix4x4 projectionMatrix;

	Vector3 rotation_ = { 0, 0, 0 };

	Matrix4x4 matRot_;

	Vector3 translation_ = { 0, 0, -50 };

	Vector3 target_ = { 0.0f, 0.0f, 0.0f };  
	float distance_ = 50.0f;

	Transform cameraTransform{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f},  {0.0f,0.0f,-10.0f} };

	Vector2 preMousePosition_ = { 0.0f, 0.0f };


};