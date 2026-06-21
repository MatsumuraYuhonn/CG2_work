#pragma once

struct Vector4 {
	float x;
	float y;
	float z;
	float w;
};

struct Vector3 {
	float x;
	float y;
	float z;
};

struct Vector2 {
	float x;
	float y;
};

struct Matrix4x4;


Vector3 Add(const Vector3& v1, const Vector3& v2);

Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);
