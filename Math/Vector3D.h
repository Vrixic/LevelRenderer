#pragma once
#include <corecrt_math.h>

/* Row vector */
struct Vector3D
{
public:
	float X;
	float Y;
	float Z;

public:
	inline Vector3D();

	inline Vector3D(float val);

	inline Vector3D(float x, float y, float z);

public:
	/* Unary operator overloads */

	inline Vector3D operator+(const Vector3D& v) const;

	inline Vector3D operator-(const Vector3D& v) const;

	/* Returns the negated copy of vector */
	inline Vector3D operator-() const;

	inline Vector3D operator*(float scalar) const;

	inline Vector3D operator/(float scalar) const;

	inline Vector3D operator*(const Vector3D& v) const;

	inline Vector3D operator/(const Vector3D& v) const;

	inline Vector3D operator+=(const Vector3D& v);

	inline Vector3D operator-=(const Vector3D& v);

	inline Vector3D operator*=(float scalar);

	inline Vector3D operator/=(float scalar);

	inline Vector3D operator*=(const Vector3D& v);

	inline Vector3D operator/=(const Vector3D& v);

public:
	inline static float DotProduct(const Vector3D& a, const Vector3D& b);

	inline static Vector3D CrossProduct(const Vector3D& a, const Vector3D& b);

	inline float Length() const;

	inline float LengthSquared() const;

	inline void Normalize();
};

inline Vector3D::Vector3D()
	: X(0.0f), Y(0.0f), Z(0.0f) {}

inline Vector3D::Vector3D(float val)
	: X(val), Y(val), Z(val) {}

inline Vector3D::Vector3D(float x, float y, float z)
	: X(x), Y(y), Z(z) {}

inline Vector3D Vector3D::operator+(const Vector3D& v) const
{
	return Vector3D(X + v.X, Y + v.Y, Z + v.Z);
}

inline Vector3D Vector3D::operator-(const Vector3D& v) const
{
	return Vector3D(X - v.X, Y - v.Y, Z - v.Z);
}

inline Vector3D Vector3D::operator-() const
{
	return Vector3D(-X, -Y, -Z);
}

inline Vector3D Vector3D::operator*(float scalar) const
{
	return Vector3D(X * scalar, Y * scalar, Z * scalar);
}

inline Vector3D Vector3D::operator/(float scalar) const
{
	float r = 1.0f / scalar;
	return Vector3D(X * r, Y * r, Z * r);
}

inline Vector3D Vector3D::operator*(const Vector3D& v) const
{
	return Vector3D(X * v.X, Y * v.Y, Z * v.Z);
}

inline Vector3D Vector3D::operator/(const Vector3D& v) const
{
	return Vector3D(X / v.X, Y / v.Y, Z / v.Z);
}

inline Vector3D Vector3D::operator+=(const Vector3D& v)
{
	X += v.X;
	Y += v.Y;
	Z += v.Z;

	return *this;
}

inline Vector3D Vector3D::operator-=(const Vector3D& v)
{
	X -= v.X;
	Y -= v.Y;
	Z -= v.Z;

	return *this;
}

inline Vector3D Vector3D::operator*=(float scalar)
{
	X *= scalar;
	Y *= scalar;
	Z *= scalar;

	return *this;
}

inline Vector3D Vector3D::operator/=(float scalar)
{
	float r = 1.0f / scalar;
	X *= r;
	Y *= r;
	Z *= r;

	return *this;
}

inline Vector3D Vector3D::operator*=(const Vector3D& v)
{
	X *= v.X;
	Y *= v.Y;
	Z *= v.Z;

	return *this;
}

inline Vector3D Vector3D::operator/=(const Vector3D& v)
{
	X /= v.X;
	Y /= v.Y;
	Z /= v.Z;

	return *this;
}

inline float Vector3D::DotProduct(const Vector3D& a, const Vector3D& b)
{
	return (a.X * b.X + a.Y * b.Y + a.Z * b.Z);
}

inline Vector3D Vector3D::CrossProduct(const Vector3D& a, const Vector3D& b)
{
	return Vector3D(
		a.Y * b.Z - a.Z * b.Y,
		a.Z * b.X - a.X * b.Z,
		a.X * b.Y - a.Y * b.X
	);
}

inline float Vector3D::Length() const
{
	return sqrtf(X * X + Y * Y + Z * Z);
}

inline float Vector3D::LengthSquared() const
{
	return (X * X + Y * Y + Z * Z);
}

inline void Vector3D::Normalize()
{
	const float Magnitude = (1.0f / Length());
	X *= Magnitude;
	Y *= Magnitude;
	Z *= Magnitude;
}
