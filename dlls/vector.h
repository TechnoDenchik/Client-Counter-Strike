/*
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by the
*   Free Software Foundation; either version 2 of the License, or (at
*   your option) any later version.
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software Foundation,
*   Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*   In addition, as a special exception, the author gives permission to
*   link the code of this program with the Half-Life Game Engine ("HL
*   Engine") and Modified Game Libraries ("MODs") developed by Valve,
*   L.L.C ("Valve").  You must obey the GNU General Public License in all
*   respects for all of the code used other than the HL Engine and MODs
*   from Valve.  If you modify this file, you may extend this exception
*   to your version of the file, but you are not obligated to do so.  If
*   you do not wish to do so, delete this exception statement from your
*   version.
*
*/

#ifndef VECTOR_H
#define VECTOR_H
#ifdef _WIN32
#pragma once
#endif

class Vector2D
{
public:
	vec_t x, y;
	Vector2D() : x(0.0), y(0.0) {}
	Vector2D(float X, float Y) : x(0.0), y(0.0)
	{
		x = X;
		y = Y;
	}
	Vector2D operator+(const Vector2D &v) const
	{
		return Vector2D(x + v.x, y + v.y);
	}
	Vector2D operator-(const Vector2D &v) const
	{
		return Vector2D(x - v.x, y - v.y);
	}
	Vector2D operator*(float fl) const
	{
		return Vector2D((vec_t)(x * fl), (vec_t)(y * fl));
	}
	Vector2D operator/(float fl) const
	{
		return Vector2D((vec_t)(x / fl), (vec_t)(y / fl));
	}
	Vector2D operator/=(float fl) const
	{
		return Vector2D((vec_t)(x / fl), (vec_t)(y / fl));
	}

	float Length() const
	{
		return sqrt((float)(x * x + y * y));
	}
	float LengthSquared() const
	{
		return (x * x + y * y);
	}
	operator float*()
	{
		return &x;
	}
	operator const float*() const
	{
		return &x;
	}
	Vector2D Normalize() const
	{
		float flLen = Length();
		if (!flLen)
			return Vector2D(0, 0);

		flLen = 1 / flLen;

		return Vector2D((vec_t)(x * flLen), (vec_t)(y * flLen));
	}
	bool IsLengthLessThan(float length) const
	{
		return (LengthSquared() < length * length);
	}
	bool IsLengthGreaterThan(float length) const
	{
		return (LengthSquared() > length * length);
	}
	float NormalizeInPlace()
	{
		float flLen = Length();
		if (flLen > 0.0)
		{
			x = (vec_t)(1 / flLen * x);
			y = (vec_t)(1 / flLen * y);
		}
		else
		{
			x = 1.0;
			y = 0.0;
		}
		return flLen;
	}
	bool IsZero(float tolerance = 0.01f) const
	{
		return (x > -tolerance && x < tolerance &&
			y > -tolerance && y < tolerance);
	}
};

#ifdef DotProduct
#undef DotProduct
#endif
inline float DotProduct(const Vector2D &a, const Vector2D &b)
{
	return (a.x * b.x + a.y * b.y);
}

inline Vector2D operator*(float fl, const Vector2D &v)
{
	return v * fl;
}

class Vector
{
public:
	vec_t x, y, z;
	Vector() : x(0.0), y(0.0), z(0.0) {}
	Vector(float X, float Y, float Z) : x(0.0), y(0.0), z(0.0)
	{
		x = X;
		y = Y;
		z = Z;
	}
	Vector(const Vector &v) : x(0.0), y(0.0), z(0.0)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}
	Vector(const float rgfl[3]) : x(0.0), y(0.0), z(0.0)
	{
		x = rgfl[0];
		y = rgfl[1];
		z = rgfl[2];
	}
	Vector operator-() const
	{
		return Vector(-x, -y, -z);
	}
	int operator==(const Vector &v) const
	{
		return x == v.x && y == v.y && z == v.z;
	}
	int operator!=(const Vector &v) const
	{
		return !(*this == v);
	}
	Vector operator+(const Vector &v) const
	{
		return Vector(x + v.x, y + v.y, z + v.z);
	}
	Vector operator-(const Vector &v) const
	{
		return Vector(x - v.x, y - v.y, z - v.z);
	}
	Vector operator*(float fl) const
	{
		return Vector((vec_t)(x * fl), (vec_t)(y * fl), (vec_t)(z * fl));
	}
	Vector operator/(float fl) const
	{
		return Vector((vec_t)(x / fl), (vec_t)(y / fl), (vec_t)(z / fl));
	}
	Vector operator/=(float fl) const
	{
		return Vector((vec_t)(x / fl), (vec_t)(y / fl), (vec_t)(z / fl));
	}
	void CopyToArray(float *rgfl) const
	{
		rgfl[0] = x;
		rgfl[1] = y;
		rgfl[2] = z;
	}
	float Length() const
	{
		float x1 = (float)x;
		float y1 = (float)y;
		float z1 = (float)z;

		return sqrt(x1 * x1 + y1 * y1 + z1 * z1);

		//return sqrt((float)(x * x + y * y + z * z));
	}
	float LengthSquared() const
	{
		return (x * x + y * y + z * z);
	}
	operator float*()
	{
		return &x;
	}
	operator const float*() const
	{
		return &x;
	}
   
	Vector Normalize()
	{
		float flLen = Length();
		if (flLen == 0)
			return Vector(0, 0, 1);

		flLen = 1 / flLen;
		return Vector(x * flLen, y * flLen, z * flLen);
	}

	// for out precision normalize
	Vector NormalizePrecision()
	{
		return Normalize();
	}
   
	Vector2D Make2D() const
	{
		Vector2D Vec2;
		Vec2.x = x;
		Vec2.y = y;
		return Vec2;
	}
	float Length2D() const
	{
		return sqrt((float)(x * x + y * y));
	}
	bool IsLengthLessThan(float length) const
	{
		return (LengthSquared() < length * length);
	}
	bool IsLengthGreaterThan(float length) const
	{
		return (LengthSquared() > length * length);
	}
   
	float NormalizeInPlace()
	{
		float flLen = Length();

		if (flLen > 0)
		{
			x = (vec_t)(1 / flLen * x);
			y = (vec_t)(1 / flLen * y);
			z = (vec_t)(1 / flLen * z);
		}
		else
		{
			x = 0;
			y = 0;
			z = 1;
		}

		return flLen;
	}
	template<typename T>
	float NormalizeInPlace()
	{
		T flLen = Length();

		if (flLen > 0)
		{
			x = (vec_t)(1 / flLen * x);
			y = (vec_t)(1 / flLen * y);
			z = (vec_t)(1 / flLen * z);
		}
		else
		{
			x = 0;
			y = 0;
			z = 1;
		}

		return flLen;
	}
   
	bool IsZero(float tolerance = 0.01f) const
	{
		return (x > -tolerance && x < tolerance &&
			y > -tolerance && y < tolerance &&
			z > -tolerance && z < tolerance);
	}
};

inline Vector operator*(float fl, const Vector &v)
{
	return v * fl;
}

inline float DotProduct(const Vector &a, const Vector &b)
{
	return (a.x * b.x + a.y * b.y + a.z * b.z);
}

inline float DotProduct2D(const Vector &a, const Vector &b)
{
	return (a.x * b.x + a.y * b.y);
}

inline Vector CrossProduct(const Vector &a, const Vector &b)
{
	return Vector(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

template<class T>
inline void SWAP(T &first, T &second)
{
	T temp = first;
	first = second;
	second = temp;
}

template<
	typename X,
	typename Y,
	typename Z,
	typename LenType
>
inline LenType LengthSubtract(Vector vecStart, Vector vecDest)
{
	X floatX = (vecDest.x - vecStart.x);
	Y floatY = (vecDest.y - vecStart.y);
	Z floatZ = (vecDest.z - vecStart.z);

	return sqrt((float)(floatX * floatX + floatY * floatY + floatZ * floatZ));
}

template<
	typename X,
	typename Y,
	typename Z,
	typename LenType
>
inline Vector NormalizeSubtract(Vector vecStart, Vector vecDest)
{
	Vector dir;

	X floatX = (vecDest.x - vecStart.x);
	Y floatY = (vecDest.y - vecStart.y);
	Z floatZ = (vecDest.z - vecStart.z);

	LenType flLen = sqrt((float)(floatX * floatX + floatY * floatY + floatZ * floatZ));

	if (flLen == 0.0)
	{
		dir = Vector(0, 0, 1);
	}
	else
	{
		flLen = 1.0 / flLen;

		dir.x = (vec_t)(floatX * flLen);
		dir.y = (vec_t)(floatY * flLen);
		dir.z = (vec_t)(floatZ * flLen);
	}
	return dir;
}

template<typename X, typename Y, typename LenType>
inline Vector NormalizeMulScalar(Vector2D vec, float scalar)
{
	LenType flLen;
	X floatX;
	Y floatY;

	flLen = (LenType)vec.Length();

	if (flLen <= 0.0)
	{
		floatX = 1;
		floatY = 0;
	}
	else
	{
		flLen = 1 / flLen;

		floatX = vec.x * flLen;
		floatY = vec.y * flLen;
	}

	return Vector((vec_t)(floatX * scalar), (vec_t)(floatY * scalar), 0);
}

#endif // VECTOR_H
