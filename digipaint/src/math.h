#pragma once

#include <math.h>

#define PI 3.14159265
#define DEG2RAD (PI/180.0f)
#define RAD2DEG (180.0f/PI)

template<typename T>
inline T min(T a, T b) { return a < b ? a : b; }

template<typename T>
inline T max(T a, T b) { return a > b ? a : b; }

template<typename T>
inline T clamp(T value, T min, T max) {
	return value < min ? min : value > max ? max : value;
}

template<typename T>
inline T lerp(T a, T b, float t)
{
	return a + (b - a) * t;
}

struct Vector
{
	float x, y;

	inline static Vector X() { return Vector{ 1,0 }; }
	inline static Vector Y() { return Vector{ 0,1 }; }

	inline Vector operator + (Vector v) { return Vector{ x + v.x, y + v.y }; }
	inline Vector operator - (Vector v) { return Vector{ x - v.x, y - v.y }; }
	inline Vector operator * (Vector v) { return Vector{ x*v.x, y*v.y }; }
	inline Vector operator / (Vector v) { return Vector{ x / v.x, y / v.y }; }

	inline void operator += (Vector v) { *this = *this + v; }
	inline void operator -= (Vector v) { *this = *this - v; }
	inline void operator *= (Vector v) { *this = *this * v; }
	inline void operator /= (Vector v) { *this = *this / v; }

	inline Vector operator * (float f) { return Vector{ x*f, y*f }; }
	inline Vector operator / (float f) { return Vector{ x / f, y / f }; }

	inline void operator *= (float f) { *this = *this * f; }
	inline void operator /= (float f) { *this = *this / f; }

	inline Vector operator - () { return Vector{ -x, -y }; }

	inline bool operator == (Vector v) { return x == v.x && y == v.y; }
	inline bool operator != (Vector v) { return !(*this == v); }

	inline float length() { return sqrtf(x*x + y * y); }
	inline float squared_length() { return x * x + y * y; }

	//TODO: Handle division by zero
	inline Vector normalized() { return *this / length(); }

	inline float dot(Vector v) { return x * v.x + y * v.y; }

	inline Vector rotated(float angle)
	{
		return Vector
		{
			x * cosf(angle) - y * sinf(angle),
			x * sinf(angle) + y * cosf(angle)
		};
	}

	inline float angle()
	{
		return atan2f(y, x);
	}

	inline Vector clamped(Vector min, Vector max)
	{
		return Vector{
			clamp(x, min.x, max.x),
			clamp(y, min.y, max.y)
		};
	}
};

struct Matrix
{
	Vector i, j, t;

	inline static Matrix Identity() { return Matrix{ 1,0,0,1,0,0 }; }

	inline Vector operator * (Vector v)
	{
		return Vector
		{
			v.dot({i.x, j.x}) + t.x,
			v.dot({i.y, j.y}) + t.y,
		};
	}

	inline Matrix operator * (Matrix m)
	{
		return Matrix
		{
			Matrix{ i,j,{0} } *m.i,
			Matrix{ i,j,{0} } *m.j,
			Matrix{ i,j, t  } *m.t
		};
	}

	inline void translate(Vector translation) { t += translation; }

	inline void scale(Vector scale)
	{
		Matrix scale_matrix = {
			scale.x,0,
			0,scale.y,
			0,0
		};
		*this = scale_matrix * (*this);
	}

	inline void rotate(float angle)
	{
		Matrix rotation = Matrix::Identity();
		rotation.i = i.rotated(angle);
		rotation.j = j.rotated(angle);

		*this = rotation * *this;
	}

	inline void invert()
	{
		float i_length = 1.0f / i.length();
		float j_length = 1.0f / j.length();
		i = i.normalized();
		j = j.normalized();

		Matrix scale = {
			i_length,0,
			0,j_length,
			0,0,
		};
		Matrix rotation = {
			i.x,j.x,
			i.y,j.y,
			0,0
		};
		Matrix translation = {
			1,0,
			0,1,
			-t
		};

		*this = scale * rotation * translation;
	}

	inline Matrix inverse()
	{
		Matrix m = Matrix(*this);
		m.invert();
		return m;
	}

	inline void to_float9(float matrix[9])
	{
		float m[9] = {
			i.x,i.y,0,
			j.x,j.y,0,
			t.x,t.y,1
		};

		memcpy(matrix, m, sizeof(float) * 9);
	}
};

bool line_intersection(Vector a1, Vector a2, Vector b1, Vector b2, Vector& intersection)
{
	//Line a
	float aA = a2.y - a1.y;
	float aB = a1.x - a2.x;
	float aC = aA * a1.x + aB * a1.y;

	//Line b
	float bA = b2.y - b1.y;
	float bB = b1.x - b2.x;
	float bC = bA * b1.x + bB * b1.y;

	float determinant = aA * bB - bA * aB;

	if (determinant == 0.0)
	{
		//Lines are parallel
		return false;
	}
	else
	{
		intersection = Vector
		{
			(bB * aC - aB * bC) / determinant,
			(aA * bC - bA * aC) / determinant
		};

		return true;
	}
}
