
// tpbds -- Standard math primitives.

// To do:
// - Complex number.
// - Quaternion.
// - Fix fraction type:
//   + Implement subtract.
//   + Add assignment operator.
//   + Signed/negative fractions?
//   + Support for mixed numbers?

#ifndef _MATH_H_
#define _MATH_H_

// -- Common floating point values. --

const float kPI = 3.1415926535897932384626433832795f;
const float kHalfPI = kPI * 0.5f;
const float kEpsilon = 5.96e-08f; // Max. error for single precision (32-bit).

// -- Misc. utility functions. --

#define MIN std::min
#define MAX std::max

// Realistic floating point compare.
inline bool comparef(float A, float B)
{
	return fabsf(A - B) < kEpsilon;
}

// Floating point random.
// To be deprecated because it has very poor distribution.
inline float randf(float range)
{
	return range * ((float) rand() / (float) RAND_MAX);
}

// Floating point clamp.
inline float clampf(float value, float floor, float ceiling)
{
	value = std::min(value, ceiling);
	value = std::max(value, floor);
	return value;
}

// Floating point saturate (clamp to [0, 1] range).
inline float saturatef(float value)
{
	return clampf(value, 0.f, 1.f);
}

// Integer greatest common divisor.
inline unsigned int gcdi(unsigned int A, unsigned int B)
{
	return (!B) ? A : gcdi(B, A % B);
}

// Linear interpolate.
template<typename T> inline const T lerpf(const T &A, const T &B, float factor)
{
	TPB_ASSERT(factor >= 0.f && factor <= 1.f);
	return A*(1.f - factor) + B*factor;
}

// -- Fraction (http://en.wikipedia.org/wiki/Fraction_(mathematics). --

class Fraction
{
private:
	static const Fraction Add(const Fraction &A, const Fraction &B)
	{
//		const unsigned int scMultiple = SmallestCommonMultiple(A.GetD(), B.GetD());
//		return Fraction(A.GetN() * (scMultiple / A.GetD()) + B.GetN() * (scMultiple / B.GetD()), scMultiple);

		// Slightly optimized (ex. 5:12 + 11:18):
		// quotB = denB/GCD = 18/6 = 3 = 36/12 = LCD/denA
		// quotA = denA/GCD = 12/6 = 2 = 36/18 = LCD/denB
		// newNum = numA*quotB + numB*quotA = 5*3 + 11*2 = 37
		// newDen = quotA*denB = 12/6 * 18 = 2*18 = 36
		const unsigned int gcDivisor = GreatestCommonDivisor(A.GetD(), B.GetD());
		const unsigned int quotB = B.GetD() / gcDivisor;
		const unsigned int quotA = A.GetD() / gcDivisor;
		return Fraction(A.GetN()* quotB + B.GetN() * quotA, quotA * B.GetD());
	}

	static const Fraction Sub(const Fraction &A, const Fraction &B)
	{
		TPB_IMPLEMENT // First evaluate the use of negative fractions and how to implement them.
	}

	static const Fraction Mul(const Fraction &A, const Fraction &B)
	{
		return Fraction(A.GetN()*B.GetN(), A.GetD()*B.GetD());
	}

	static const Fraction Div(const Fraction &A, const Fraction &B)
	{
		return Mul(A, B.Reciprocal());
	}

	static unsigned int GreatestCommonDivisor(unsigned int A, unsigned int B)
	{
		return (!B) ? A : GreatestCommonDivisor(B, A % B);
	}

	static unsigned int SmallestCommonMultiple(unsigned int A, unsigned int B)
	{
		const unsigned int gcDivisor = GreatestCommonDivisor(A, B);
		return A/gcDivisor * B; // = A/C * B = A*B / C
	}

public:
	Fraction() :
		m_numerator(1),
		m_denominator(1) {}

	Fraction(unsigned int numerator, unsigned int denominator) :
		m_numerator(numerator),
		m_denominator(denominator) 
	{
		TPB_ASSERT(denominator != 0);
	}

	~Fraction() {}

	// Unlike in other math primitives, GetN() and GetD() are prefixed because there are no non-const counterparts.
	// Abbreviated for readability.
	unsigned int GetN() const { return m_numerator; }
	unsigned int GetD() const { return m_denominator; }

	// Preferred above a cast operator.
	float GetRatio() const { return (float) GetN() / (float) GetD(); }

	const Fraction Normalize() const
	{
		const unsigned int gcDivisor = GreatestCommonDivisor(m_numerator, m_denominator);
		return Fraction(GetN() / gcDivisor, GetD() / gcDivisor);
	}

	const Fraction Reciprocal() const 
	{
		return Fraction(GetD(), GetN());
	}
	
	bool IsMixedNumber() const
	{
		return GetN() <= GetD();
	}

	const Fraction operator + (const Fraction &B) const { return Add(*this, B); }
	const Fraction operator - (const Fraction &B) const { return Sub(*this, B); }
	const Fraction operator * (const Fraction &B) const { return Mul(*this, B); }
	const Fraction operator / (const Fraction &B) const { return Div(*this, B); }

	Fraction &operator += (const Fraction &B) { return *this = *this + B; }
	Fraction &operator -= (const Fraction &B) { return *this = *this - B; }
	Fraction &operator *= (const Fraction &B) { return *this = *this * B; }
	Fraction &operator /= (const Fraction &B) { return *this = *this / B; }

	bool operator ==(const Fraction &B) const
	{
		const Fraction normA = Normalize();
		const Fraction normB = B.Normalize();
		return normA.GetN() == normB.GetN() && normA.GetD() == normB.GetD();
	}

	bool operator !=(const Fraction &B) const
	{
		return !(*this == B);
	}

private:
	unsigned int m_numerator;
	unsigned int m_denominator;
};

//-hier was ik gebleven

// -- 2D vector --

class Vector2
{
private:
	static const Vector2 Add(const Vector2 &A, const Vector2 &B) { return Vector2(A.m_X + B.m_X, A.m_Y + B.m_Y); }
	static const Vector2 Add(const Vector2 &A, float B) { return Vector2(A.m_X + B, A.m_Y + B); }

	static const Vector2 Sub(const Vector2 &A, const Vector2 &B) { return Vector2(A.m_X - B.m_X, A.m_Y - B.m_Y); }
	static const Vector2 Sub(const Vector2 &A, float B) { return Vector2(A.m_X - B, A.m_Y - B); }

	static const Vector2 Div(const Vector2 &A, const Vector2 &B) 
	{
		return Vector2(A.m_X / B.m_X, A.m_Y / B.m_Y);
	}

	static const Vector2 Scale(const Vector2 &A, const Vector2 &B) { return Vector2(A.m_X * B.m_X, A.m_Y * B.m_Y); }
	static const Vector2 Scale(const Vector2 &A, float B) { return Vector2(A.m_X * B, A.m_Y * B); }

	static float DotProduct(const Vector2 &A, const Vector2 &B)
	{
		return (A.m_X * B.m_X) + (A.m_Y * B.m_Y);
	}

public:
	Vector2() {}
	~Vector2() {}

	explicit Vector2(float XY) :
		m_X(XY), m_Y(XY) {}
	
	Vector2(float X, float Y) :
		m_X(X), m_Y(Y) {}

	Vector2(const float *pXY) :
		m_X(pXY[0]), m_Y(pXY[1]) {}

	const float *GetData() const { return &m_X; }

	float &X() { return m_X; }
	float &Y() { return m_Y; }
	
	float X() const { return m_X; }
	float Y() const { return m_Y; }

	float Magnitude() const 
	{
		return sqrtf(DotProduct(*this, *this));
	}

	const Vector2 Normalize() const
	{
		return Scale(*this, 1.f / Magnitude());
	}

	float Angle(const Vector2 &B) const
	{
		return acosf(clampf(DotProduct(*this, B), -1.f, 1.f));
	}

	const Vector2 Project(const Vector2 &B) const
	{
		return Scale(*this, DotProduct(*this, B));
	}

	const Vector2 Reflect(const Vector2 &normal) const
	{
		const float R = 2.f * DotProduct(*this, normal);
		return Sub(*this, Scale(normal, R));
	}

	const Vector2 Scale(const Vector2 &B) const 
	{
		return Scale(*this, B);
	}

	const Vector2 Perpendicular() const 
	{
		return Vector2(-m_Y, m_X);
	}

	const Vector2 operator +(const Vector2 &B) const { return Add(*this, B); }
	const Vector2 operator +(float B) const { return Add(*this, B); }
	const Vector2 operator -(const Vector2 &B) const { return Sub(*this, B); }
	const Vector2 operator -(float B) const { return Sub(*this, B); }
	const Vector2 operator /(const Vector2 &B) const { return Div(*this, B); }
	const Vector2 operator /(float B) const { return Scale(*this, 1.f / B); }
	const Vector2 operator *(float B) const { return Scale(*this, B); }

	float operator *(const Vector2 &B) const { return DotProduct(*this, B); }

	Vector2 & operator +=(const Vector2 &B) { return *this = *this + B; }
	Vector2 & operator +=(float B) { return *this = *this + B; }
	Vector2 & operator -=(const Vector2 &B) { return *this = *this - B; }
	Vector2 & operator -=(float B) { return *this = *this - B; }
	Vector2 & operator /=(const Vector2 &B) { return *this = *this / B; }
	Vector2 & operator /=(float B) { return *this = *this / B; }
	Vector2 & operator *=(float B) { return *this = *this * B; }

	float & operator [](unsigned int index) { TPB_ASSERT(index < 2); return *(&m_X + index); }
	float operator [](unsigned int index) const { TPB_ASSERT(index < 2); return *(&m_X + index); }

	bool operator ==(const Vector2 &B) const
	{
		return comparef(m_X, B.m_X) && comparef(m_Y, B.m_Y);
	}

	bool operator !=(const Vector2 &B) const
	{
		return !(*this == B);
	}
	
	float m_X, m_Y;
};

// -- 3D vector --

class Vector3
{
private:
	static const Vector3 Add(const Vector3 &A, const Vector3 &B) { return Vector3(A.m_X + B.m_X, A.m_Y + B.m_Y, A.m_Z + B.m_Z); }
	static const Vector3 Add(const Vector3 &A, float B) { return Vector3(A.m_X + B, A.m_Y + B, A.m_Z - B); }

	static const Vector3 Sub(const Vector3 &A, const Vector3 &B) { return Vector3(A.m_X - B.m_X, A.m_Y - B.m_Y, A.m_Z - B.m_Z); }
	static const Vector3 Sub(const Vector3 &A, float B) { return Vector3(A.m_X - B, A.m_Y - B, A.m_Z - B); }

	static const Vector3 Div(const Vector3 &A, const Vector3 &B)
	{
		return Vector3(A.m_X / B.m_X, A.m_Y / B.m_Y, A.m_Z / B.m_Z);
	}

	static const Vector3 Scale(const Vector3 &A, const Vector3 &B) { return Vector3(A.m_X * B.m_X, A.m_Y * B.m_Y, A.m_Z * B.m_Z); }
	static const Vector3 Scale(const Vector3 &A, float B) { return Vector3(A.m_X * B, A.m_Y * B, A.m_Z * B); }

	static const Vector3 CrossProduct(const Vector3 &A, const Vector3 &B)
	{
		return Vector3(A.m_Y * B.m_Z - A.m_Z * B.m_Y, A.m_Z * B.m_X - A.m_X * B.m_Z, A.m_X * B.m_Y - A.m_Y * B.m_X);
	}

	static float DotProduct(const Vector3 &A, const Vector3 &B)
	{
		return (A.m_X * B.m_X) + (A.m_Y * B.m_Y) + (A.m_Z * B.m_Z);
	}

public:
	Vector3() {}
	~Vector3() {}

	explicit Vector3(float XYZ) :
		m_X(XYZ), m_Y(XYZ), m_Z(XYZ) {}
	
	Vector3(float X, float Y, float Z) :
		m_X(X), m_Y(Y), m_Z(Z) {}

	explicit Vector3(const Vector2 &V) :
		m_X(V.m_X), m_Y(V.m_Y), m_Z(1.f) {}

	explicit Vector3(const float *pXYZ) :
		m_X(pXYZ[0]), m_Y(pXYZ[1]), m_Z(pXYZ[2]) {}

	const float *GetData() const { return &m_X; }

	float &X() { return m_X; }
	float &Y() { return m_Y; }
	float &Z() { return m_Z; }

	float X() const { return m_X; }
	float Y() const { return m_Y; }
	float Z() const { return m_Z; }

	float Magnitude() const 
	{
		return sqrtf(DotProduct(*this, *this));
	}

	float MagnitudeSqr() const 
	{
		return DotProduct(*this, *this);
	}

	const Vector3 Normalize() const
	{
		return Scale(*this, 1.f / Magnitude());
	}

	float Angle(const Vector3 &B) const
	{
		return acosf(clampf(DotProduct(*this, B), -1.f, 1.f));
	}

	const Vector3 Project(const Vector3 &B) const
	{
		return Scale(*this, DotProduct(*this, B));
	}

	const Vector3 Reflect(const Vector3 &normal) const
	{
		const float R = 2.f * DotProduct(*this, normal);
		return Sub(*this, Scale(normal, R));
	}

	const Vector3 Scale(const Vector3 &B) const 
	{
		return Scale(*this, B);
	}
	
	// I've overloaded the %-operator to perform a cross product.
	// This isn't standard so just in case I've added this function as well.
	const Vector3 CrossProduct(const Vector3 &B) const
	{
		return CrossProduct(*this, B);
	}

	const Vector3 operator +(const Vector3 &B) const { return Add(*this, B); }
	const Vector3 operator +(float B) const { return Add(*this, B); }
	const Vector3 operator -(const Vector3 &B) const { return Sub(*this, B); }
	const Vector3 operator -(float B) const { return Sub(*this, B); }
	const Vector3 operator /(const Vector3 &B) const { return Div(*this, B); }
	const Vector3 operator /(float B) const { return Scale(*this, 1.f / B); }
	const Vector3 operator *(float B) const { return Scale(*this, B); }

	float operator *(const Vector3 &B) const { return DotProduct(*this, B); }
	const Vector3 operator %(const Vector3 &B) const { return CrossProduct(*this, B); }

	Vector3 & operator +=(const Vector3 &B) { return *this = *this + B; }
	Vector3 & operator +=(float B) { return *this = *this + B; }
	Vector3 & operator -=(const Vector3 &B) { return *this = *this - B; }
	Vector3 & operator -=(float B) { return *this = *this - B; }
	Vector3 & operator /=(const Vector3 &B) { return *this = *this / B; }
	Vector3 & operator /=(float B) { return *this = *this / B; }
	Vector3 & operator *=(float B) { return *this = *this * B; }

	Vector3 & operator %=(const Vector3 &B) { return *this = *this % B; }

	float & operator [](unsigned int index) { TPB_ASSERT(index < 3); return *(&m_X + index); }
	float operator [](unsigned int index) const { TPB_ASSERT(index < 3); return *(&m_X + index); }

	bool operator ==(const Vector3 &B) const
	{ 
		return comparef(m_X, B.m_X) && comparef(m_Y, B.m_Y) && comparef(m_Z,  B.m_Z);
	}

	bool operator !=(const Vector3 &B) const
	{
		return !(*this == B);
	}

	float m_X, m_Y, m_Z;
};

// -- 4D vector --

// This vector is not fully implemented because it is hardly used.
class Vector4
{
public:
	Vector4() {}
	~Vector4() {}

	explicit Vector4(float XYZW) :
		m_X(XYZW), m_Y(XYZW), m_Z(XYZW), m_W(XYZW) {}
	
	Vector4(float X, float Y, float Z) :
		m_X(X), m_Y(Y), m_Z(Z), m_W(1.f) {}

	// I've found it to be convenient enough to keep this constructor non-explicit.
	Vector4(const Vector3 &V) :
		m_X(V.m_X), m_Y(V.m_Y), m_Z(V.m_Z), m_W(1.f) {}

	Vector4(const Vector3 &V, float W) :
		m_X(V.m_X), m_Y(V.m_Y), m_Z(V.m_Z), m_W(W) {}

	Vector4(float X, float Y, float Z, float W) :
		m_X(X), m_Y(Y), m_Z(Z), m_W(W) {}

	const float *GetData() const { return &m_X; }

	float &X() { return m_X; }
	float &Y() { return m_Y; }
	float &Z() { return m_Z; }
	float &W() { return m_W; }
	
	float X() const { return m_X; }
	float Y() const { return m_Y; }
	float Z() const { return m_Z; }
	float W() const { return m_W; }

	operator Vector3() const { return Vector3(m_X, m_Y, m_Z); }

	float m_X, m_Y, m_Z	, m_W;
};

// -- row-major 4*4 matrix (designed for 3D transformations) --

class Matrix4x4
{
public:
	static const Matrix4x4 Identity();
	static const Matrix4x4 Scaling(const Vector3 &V);
	static const Matrix4x4 Translation(const Vector3 &V);
	static const Matrix4x4 RotationX(float thetaRad);
	static const Matrix4x4 RotationY(float thetaRad);
	static const Matrix4x4 RotationZ(float thetaRad);
	static const Matrix4x4 RotationYPR(const Vector3 &eulerRad);
	static const Matrix4x4 RotationAxis(const Vector3 &axis, float thetaRad);
	static const Matrix4x4 View(const Vector3 &from, const Vector3 &to, const Vector3 &up);
	static const Matrix4x4 PerspectiveProjection(float yFieldOfViewRad, float aspectRatio, float zNear, float zFar);
	static const Matrix4x4 OrthoProjection(float Left, float Right, float Bottom, float Top, float Near, float Far);
	static const Matrix4x4 FromArray(const float floatArr[16]);

	Matrix4x4() {}
	~Matrix4x4() {}

	const float *GetData() const 
	{ 
		return &m_rows[0].m_X;
	}
	
	const Matrix4x4 Transpose() const;
	const Matrix4x4 Multiply(const Matrix4x4 &B) const;
	const Vector3 Multiply(const Vector3 &B) const;
	const Matrix4x4 AffineInverse() const;
	
	void SetTranslation(const Vector4 &V);
	void SetTranslation(const Vector3 &V) { SetTranslation(Vector4(V)); }

	// V' = M*V
	const Vector3 operator *(const Vector3 &B) const { return Multiply(B); }

	// M' = M*M
	const Matrix4x4 operator *(const Matrix4x4 &B) const { return Multiply(B); }
	Matrix4x4 & operator *=(const Matrix4x4 &B) { return *this = *this * B; }

	Vector4 m_rows[4];
};	

#endif // _MATH_H_
