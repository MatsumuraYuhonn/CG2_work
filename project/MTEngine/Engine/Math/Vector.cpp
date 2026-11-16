#include "Vector.h"
#include "Matrix.h"


namespace MTEngine {

	Vector3 Add(const Vector3& v1, const Vector3& v2)
	{

		Vector3 result;

		result.x = v1.x + v2.x;
		result.y = v1.y + v2.y;
		result.z = v1.z + v2.z;

		return result;
	}

	Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m) {
		Vector3 result;

		result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0];
		result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1];
		result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2];

		return result;
	}

}