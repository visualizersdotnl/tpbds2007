
// tpbds -- std. math primitives (ref. implementation)

#include "main.h"

// -- row-major 4*4 matrix --

const Matrix4x4 Matrix4x4::Identity()
{
	Matrix4x4 ID;
	ID.m_rows[0] = Vector4(1.f, 0.f, 0.f, 0.f);
	ID.m_rows[1] = Vector4(0.f, 1.f, 0.f, 0.f);
	ID.m_rows[2] = Vector4(0.f, 0.f, 1.f, 0.f);
	ID.m_rows[3] = Vector4(0.f, 0.f, 0.f, 1.f);
	return ID;
}

const Matrix4x4 Matrix4x4::Scaling(const Vector3 &V)
{
	Matrix4x4 scaling;
	scaling.m_rows[0] = Vector4(V.m_X, 0.f, 0.f, 0.f);
	scaling.m_rows[1] = Vector4(0.f, V.m_Y, 0.f, 0.f);
	scaling.m_rows[2] = Vector4(0.f, 0.f, V.m_Z, 0.f);
	scaling.m_rows[3] = Vector4(0.f, 0.f, 0.f, 1.f);
	return scaling;
}

const Matrix4x4 Matrix4x4::Translation(const Vector3 &V)
{
	Matrix4x4 translation = Matrix4x4::Identity();
	translation.SetTranslation(V);
	return translation;
}

const Matrix4x4 Matrix4x4::RotationX(float thetaRad)
{
	Matrix4x4 rotX;
	rotX.m_rows[0] = Vector4(1.f, 0.f, 0.f, 0.f);
	rotX.m_rows[1] = Vector4(0.f, cosf(thetaRad), sinf(thetaRad), 0.f);
	rotX.m_rows[2] = Vector4(0.f, -sinf(thetaRad), cosf(thetaRad), 0.f);
	rotX.m_rows[3] = Vector4(0.f, 0.f, 0.f, 1.f);
	return rotX;
}

const Matrix4x4 Matrix4x4::RotationY(float thetaRad)
{
	Matrix4x4 rotY;
	rotY.m_rows[0] = Vector4(cosf(thetaRad), 0.f, -sinf(thetaRad), 0.f);
	rotY.m_rows[1] = Vector4(0.f, 1.f, 0.f, 0.f);
	rotY.m_rows[2] = Vector4(sinf(thetaRad), 0.f, cosf(thetaRad), 0.f);	
	rotY.m_rows[3] = Vector4(0.f, 0.f, 0.f, 1.f);
	return rotY;
}

const Matrix4x4 Matrix4x4::RotationZ(float thetaRad)
{
	Matrix4x4 rotZ;
	rotZ.m_rows[0] = Vector4(cosf(thetaRad), sinf(thetaRad), 0.f, 0.f);
	rotZ.m_rows[1] = Vector4(-sinf(thetaRad), cosf(thetaRad), 0.f, 0.f);
	rotZ.m_rows[2] = Vector4(0.f, 0.f, 1.f, 0.f);
	rotZ.m_rows[3] = Vector4(0.f, 0.f, 0.f, 1.f);
	return rotZ;
}

const Matrix4x4 Matrix4x4::RotationYPR(const Vector3 &eulerRad)
{
	FIX_ME // this is obviously prone to gimbal lock
	return RotationY(eulerRad.m_Y) * RotationX(eulerRad.m_X) * RotationZ(eulerRad.m_Z);
}

const Matrix4x4 Matrix4x4::RotationAxis(const Vector3 &axis, float thetaRad) // -> Quat -> Matrix
{
	const Vector3 vAxisNormalized = axis.Normalize();

	thetaRad *= 0.5f;
	const Vector4 Quat(vAxisNormalized * sinf(thetaRad), cosf(thetaRad));
	const float XX = Quat.m_X * Quat.m_X;
	const float YY = Quat.m_Y * Quat.m_Y;
	const float ZZ = Quat.m_Z * Quat.m_Z;
	const float XY = Quat.m_X * Quat.m_Y;
	const float XZ = Quat.m_X * Quat.m_Z;
	const float YZ = Quat.m_Y * Quat.m_Z;
	const float XW = Quat.m_X * Quat.m_W;
	const float YW = Quat.m_Y * Quat.m_W;
	const float ZW = Quat.m_Z * Quat.m_W;

	Matrix4x4 result;
	result.m_rows[0] = Vector4(1.f - 2.f * (YY + ZZ), 2.f * (XY + ZW), 2.f * (XZ - YW), 0.f);
	result.m_rows[1] = Vector4(2.f * (XY - ZW), 1.f - 2.f * (XX + ZZ), 2.f * (YZ + XW), 0.f);
	result.m_rows[2] = Vector4(2.f * (XZ + YW), 2.f * (YZ - XW), 1.f - 2.f * (XX + YY), 0.f);
	result.m_rows[3] = Vector4(0.f, 0.f, 0.f, 1.f);
	return result;
}

const Matrix4x4 Matrix4x4::View(const Vector3 &from, const Vector3 &to, const Vector3 &up)
{
	TPB_ASSERT(up.Magnitude() == 1.f);
	const Vector3 zAxis = (to - from).Normalize();
	const Vector3 xAxis = (up % zAxis).Normalize();	
	const Vector3 yAxis = zAxis % xAxis;

	Matrix4x4 view;
	view.m_rows[0] = Vector4(xAxis, -(xAxis * from));
	view.m_rows[1] = Vector4(yAxis, -(yAxis * from));
	view.m_rows[2] = Vector4(zAxis, -(zAxis * from));
	view.m_rows[3] = Vector4(0.f, 0.f, 0.f, 1.f);
	return view;
}

const Matrix4x4 Matrix4x4::PerspectiveProjection(float yFieldOfViewRad, float aspectRatio, float zNear, float zFar)
{
	const float yScale = 1.f / tanf(yFieldOfViewRad * 0.5f);
	const float xScale = yScale / aspectRatio; 	

	Matrix4x4 perspProj;
	perspProj.m_rows[0] = Vector4(xScale, 0.f, 0.f, 0.f);
	perspProj.m_rows[1] = Vector4(0.f, yScale, 0.f, 0.f);
	perspProj.m_rows[2] = Vector4(0.f, 0.f, zFar / (zFar - zNear), -zNear * zFar / (zFar - zNear));
	perspProj.m_rows[3] = Vector4(0.f, 0.f, 1.f, 0.f);
	return perspProj;
}

const Matrix4x4 Matrix4x4::OrthoProjection(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    Matrix4x4 orthoProj;
    orthoProj.m_rows[0] = Vector4(2.f / (Right - Left), 0.f, 0.f, -(Right + Left) / (Right - Left));
    orthoProj.m_rows[1] = Vector4(0.f, 2.f / (Top - Bottom), 0.f, -(Top + Bottom) / (Top - Bottom));
    orthoProj.m_rows[2] = Vector4(0.f, 0.f, 2.f / (Far - Near), -(Far + Near) / (Far - Near));
    orthoProj.m_rows[3] = Vector4(0.f, 0.f, 0.f, 1.f);
    return orthoProj;
}
const Matrix4x4 Matrix4x4::FromArray(const float floatArray[16]) 
{
	TPB_ASSERT(floatArray != NULL);
	Matrix4x4 newMat;
	memcpy(&newMat, floatArray, 16 * sizeof(float));
    return newMat;
}

const Matrix4x4 Matrix4x4::Transpose() const
{
	Matrix4x4 transpose;
	transpose.m_rows[0] = Vector4(m_rows[0].m_X, m_rows[1].m_X, m_rows[2].m_X, m_rows[3].m_X);
	transpose.m_rows[1] = Vector4(m_rows[0].m_Y, m_rows[1].m_Y, m_rows[2].m_Y, m_rows[3].m_Y);
	transpose.m_rows[2] = Vector4(m_rows[0].m_Z, m_rows[1].m_Z, m_rows[2].m_Z, m_rows[3].m_Z);
	transpose.m_rows[3] = Vector4(m_rows[0].m_W, m_rows[1].m_W, m_rows[2].m_W, m_rows[3].m_W);
	return transpose;
}

const Matrix4x4 Matrix4x4::Multiply(const Matrix4x4 &B) const
{
	Matrix4x4 product;	
	product.m_rows[0].m_X = m_rows[0].m_X * B.m_rows[0].m_X + m_rows[0].m_Y * B.m_rows[1].m_X + m_rows[0].m_Z * B.m_rows[2].m_X + m_rows[0].m_W * B.m_rows[3].m_X;
	product.m_rows[0].m_Y = m_rows[0].m_X * B.m_rows[0].m_Y + m_rows[0].m_Y * B.m_rows[1].m_Y + m_rows[0].m_Z * B.m_rows[2].m_Y + m_rows[0].m_W * B.m_rows[3].m_Y;
	product.m_rows[0].m_Z = m_rows[0].m_X * B.m_rows[0].m_Z + m_rows[0].m_Y * B.m_rows[1].m_Z + m_rows[0].m_Z * B.m_rows[2].m_Z + m_rows[0].m_W * B.m_rows[3].m_Z;
	product.m_rows[0].m_W = m_rows[0].m_X * B.m_rows[0].m_W + m_rows[0].m_Y * B.m_rows[1].m_W + m_rows[0].m_Z * B.m_rows[2].m_W + m_rows[0].m_W * B.m_rows[3].m_W;
	product.m_rows[1].m_X = m_rows[1].m_X * B.m_rows[0].m_X + m_rows[1].m_Y * B.m_rows[1].m_X + m_rows[1].m_Z * B.m_rows[2].m_X + m_rows[1].m_W * B.m_rows[3].m_X;
	product.m_rows[1].m_Y = m_rows[1].m_X * B.m_rows[0].m_Y + m_rows[1].m_Y * B.m_rows[1].m_Y + m_rows[1].m_Z * B.m_rows[2].m_Y + m_rows[1].m_W * B.m_rows[3].m_Y;
	product.m_rows[1].m_Z = m_rows[1].m_X * B.m_rows[0].m_Z + m_rows[1].m_Y * B.m_rows[1].m_Z + m_rows[1].m_Z * B.m_rows[2].m_Z + m_rows[1].m_W * B.m_rows[3].m_Z;
	product.m_rows[1].m_W = m_rows[1].m_X * B.m_rows[0].m_W + m_rows[1].m_Y * B.m_rows[1].m_W + m_rows[1].m_Z * B.m_rows[2].m_W + m_rows[1].m_W * B.m_rows[3].m_W;
	product.m_rows[2].m_X = m_rows[2].m_X * B.m_rows[0].m_X + m_rows[2].m_Y * B.m_rows[1].m_X + m_rows[2].m_Z * B.m_rows[2].m_X + m_rows[2].m_W * B.m_rows[3].m_X;
	product.m_rows[2].m_Y = m_rows[2].m_X * B.m_rows[0].m_Y + m_rows[2].m_Y * B.m_rows[1].m_Y + m_rows[2].m_Z * B.m_rows[2].m_Y + m_rows[2].m_W * B.m_rows[3].m_Y;
	product.m_rows[2].m_Z = m_rows[2].m_X * B.m_rows[0].m_Z + m_rows[2].m_Y * B.m_rows[1].m_Z + m_rows[2].m_Z * B.m_rows[2].m_Z + m_rows[2].m_W * B.m_rows[3].m_Z;
	product.m_rows[2].m_W = m_rows[2].m_X * B.m_rows[0].m_W + m_rows[2].m_Y * B.m_rows[1].m_W + m_rows[2].m_Z * B.m_rows[2].m_W + m_rows[2].m_W * B.m_rows[3].m_W;
	product.m_rows[3].m_X = m_rows[3].m_X * B.m_rows[0].m_X + m_rows[3].m_Y * B.m_rows[1].m_X + m_rows[3].m_Z * B.m_rows[2].m_X + m_rows[3].m_W * B.m_rows[3].m_X;
	product.m_rows[3].m_Y = m_rows[3].m_X * B.m_rows[0].m_Y + m_rows[3].m_Y * B.m_rows[1].m_Y + m_rows[3].m_Z * B.m_rows[2].m_Y + m_rows[3].m_W * B.m_rows[3].m_Y;
	product.m_rows[3].m_Z = m_rows[3].m_X * B.m_rows[0].m_Z + m_rows[3].m_Y * B.m_rows[1].m_Z + m_rows[3].m_Z * B.m_rows[2].m_Z + m_rows[3].m_W * B.m_rows[3].m_Z;
	product.m_rows[3].m_W = m_rows[3].m_X * B.m_rows[0].m_W + m_rows[3].m_Y * B.m_rows[1].m_W + m_rows[3].m_Z * B.m_rows[2].m_W + m_rows[3].m_W * B.m_rows[3].m_W;
	return product;
}

const Vector3 Matrix4x4::Multiply(const Vector3 &B) const
{
	const float X = m_rows[0].m_X * B.m_X + m_rows[0].m_Y * B.m_Y + m_rows[0].m_Z * B.m_Z + m_rows[0].m_W;
	const float Y = m_rows[1].m_X * B.m_X + m_rows[1].m_Y * B.m_Y + m_rows[1].m_Z * B.m_Z + m_rows[1].m_W;
	const float Z = m_rows[2].m_X * B.m_X + m_rows[2].m_Y * B.m_Y + m_rows[2].m_Z * B.m_Z + m_rows[2].m_W;
	return Vector3(X, Y, Z);
}

const Matrix4x4 Matrix4x4::AffineInverse() const
{
	Matrix4x4 inverse;
	inverse.m_rows[0] = Vector4(m_rows[0].m_X, m_rows[1].m_X, m_rows[2].m_X, -m_rows[0].m_W);
	inverse.m_rows[1] = Vector4(m_rows[0].m_Y, m_rows[1].m_Y, m_rows[2].m_Y, -m_rows[1].m_W);
	inverse.m_rows[2] = Vector4(m_rows[0].m_Z, m_rows[1].m_Z, m_rows[2].m_Z, -m_rows[2].m_W);
	inverse.m_rows[3] = Vector4(0.f, 0.f, 0.f, m_rows[3].m_W);
	return inverse;
}

void Matrix4x4::SetTranslation(const Vector4 &V)
{
	m_rows[0].m_W = V.m_X;
	m_rows[1].m_W = V.m_Y;
	m_rows[2].m_W = V.m_Z;
	m_rows[3].m_W = V.m_W;
}
