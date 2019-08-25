#pragma once

#include <math.h>

#include <vector>

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

struct TriangulatedLine
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
};

void line_to_tris(std::vector<Vector> line, float width, TriangulatedLine& result )
{
	result.vertices.reserve(result.vertices.size() + line.size() * 2);
	result.indices.reserve(result.indices.size() + line.size() * 2 * 6);

	width /= 2.0;
	Vector direction;
	float length = 0;
	
	for (int i = 0; i < line.size(); i++)
	{
		Vector left;
		Vector right;

		if (i != 0)
		{
			length += (line[i] - line[i - 1]).length();
		}
		
		if (i == 0) //first element
		{
			direction = (line[i + 1] - line[i]).normalized();
			Vector side_direction = direction.rotated(-90 * DEG2RAD);
			left = line[i] + side_direction * width;
			right = line[i] - side_direction * width;
		}
		else if (i == line.size() - 1) //last element
		{
			direction = (line[i] - line[i - 1]).normalized();
			Vector side_direction = direction.rotated(-90 * DEG2RAD);
			left = line[i] + side_direction * width;
			right = line[i] - side_direction * width;
		}
		else
		{
			Vector previous_direction = direction;
			Vector current_direction = (line[i + 1] - line[i]).normalized();
			direction = current_direction;

			Vector previous_side_direction = previous_direction.rotated(-90 * DEG2RAD);
			Vector previous_left = line[i - 1] + previous_side_direction * width;
			Vector previous_right = line[i - 1] - previous_side_direction * width;

			Vector current_side_direction = current_direction.rotated(-90 * DEG2RAD);
			Vector next_left = line[i + 1] + current_side_direction * width;
			Vector next_right = line[i + 1] - current_side_direction * width;

			bool intersect = line_intersection(previous_left, previous_left + previous_direction, next_left, next_left - current_direction, left);
			intersect = intersect && line_intersection(previous_right, previous_right + previous_direction, next_right, next_right - current_direction, right);

			if (!intersect) //they are parallel
			{
				left = line[i] + current_side_direction * width;
				right = line[i] - current_side_direction * width;
			}
		}

		Vertex vertex_left = { {left.x, left.y }, {length, 0} };
		Vertex vertex_right = { {right.x, right.y }, {length, 1} };
		result.vertices.insert(result.vertices.end(), { vertex_left, vertex_right });

		if (i != 0)
		{
			//first vertex index of the new quad
			unsigned int v = result.vertices.size() - 4;
			result.indices.insert(result.indices.end(), { v, v+2, v+3, v, v+3, v+1 });
		}

	}
}
