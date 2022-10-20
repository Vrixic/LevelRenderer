#pragma once

#include <cstdlib>

class Vector3D;
class Vector4D;
class Matrix4D;

#define PI (3.1415926535897932f)

/* These Functions can be implemented in different locations if not implemented in here */
struct Math
{
public:
	template<class T>
	/* Return minimum between a and b */
	inline static T Min(T a, T b)
	{
		return a < b ? a : b;
	}

	template<class T>
	/* Return maximum between a and b */
	inline static T Max(T a, T b)
	{
		return a > b ? a : b;
	}

	template<class T>
	inline static T Clamp(T min, T max, T test)
	{
		T r = test;
		if (test > max)
		{
			r = max;
		}
		else if (test < min)
		{
			r = min;
		}

		return r;
	}

	inline static float RandomRange(float min, float max)
	{
		float r = rand() / (float)RAND_MAX;
		return (max - min) * r + min;
	}

	inline static float Lerp(float start, float end, float ratio)
	{
		return static_cast<float>(end - start) * ratio + start;
	}

	/* Converts Degrees to radians */
	inline static float DegreesToRadians(float degrees)
	{
		return static_cast<float>(((PI / 180.0f) * degrees));
	}

	/*
	* true if any spheres intersected or are inside the planes
	* false if they can be culled
	*/
	//inline static bool IntersectSpheresOnPlane(uint32 planeCount, 
	//	const Plane* planes, const Vector3D& center, float radius)
	//{
	//	float negativeRadius = -radius;
	//	for (uint32 i = 0; i < planeCount; ++i)
	//	{
	//		if (Plane::Dot(planes[i], center) <= negativeRadius)
	//		{
	//			return false;
	//		}
	//	}
	//
	//	return true;
	//}
};
