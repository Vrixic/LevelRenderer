#pragma once
#include "Math/Matrix4D.h"
#include "GatewareDefine.h"

enum {
	TOP = 0, BOTTOM, LEFT,
	RIGHT, NEARP, FARP
};

enum PlaneIntersectionResult
{
	Front,
	Back,
	Straddling
};


/*
* XYZ for the normal of the plane
* Distance is the distance of the plane -> often called distance from the origin
*/

struct Plane3
{
	Vector3D X, Y, Z;

	Plane3() { }

	Plane3(Vector3D x, Vector3D y, Vector3D z)
		: X(x), Y(y), Z(z)
	{

	}

	float distance(Vector3D v)
	{
		return Vector3D::DotProduct(X, v);
	}
};

struct Plane
{
	float X;
	float Y;
	float Z;
	float Distance;

	inline Plane() : X(0.0f), Y(0.0f), Z(0.0f), Distance(0.0f) { }

	inline Plane(float x, float y, float z, float distance)
		: X(x), Y(y), Z(z), Distance(distance) { }

	inline Plane(Vector3D& normal, float distance)
		: X(normal.X), Y(normal.Y), Z(normal.Z), Distance(distance) { }

	inline const Vector3D& GetNormal() const
	{
		return Vector3D(X, Y, Z);
	}

	/*
	* The dot product between a point and a plane is equal to the signed
	*	perpendicular distance between them
	*
	* When the normal is unit length, The dot product between a point and a plane
	*	is equal to the length of the projection of p onto n
	*
	* The sign of the distance given by The dot product between a point and a plane
	*	depends on the which side of the plane the point lies.
	*/
	inline static float Dot(const Plane& p, const Vector3D& v)
	{
		return p.X * v.X + p.Y * v.Y + p.Z * v.Z;
	}

	inline void Normalize()
	{
		float Magnitude = (X * X + Y * Y + Z * Z);
		float R = 1 / Magnitude;

		X *= R;
		Y *= R;
		Z *= R;
		Distance *= R;
	}
};

struct Frustum
{
	Plane Planes[6];

	float AspectRatio, FOV, TanFOV, FarPlaneDistance, NearPlaneDistance;

	float FarPlaneHeight, FarPlaneWidth;
	float NearPlaneHeight, NearPlaneWidth;

	/* For Debug reasons/ visuals */
	Vector3D FarPlaneTopLeft, FarPlaneTopRight, FarPlaneBottomLeft, FarPlaneBottomRight;
	Vector3D NearPlaneTopLeft, NearPlaneTopRight, NearPlaneBottomLeft, NearPlaneBottomRight;

public:
	Frustum() { }

	Frustum(float aspectRatio, float fovInDegrees, float nearPlaneDist, float farPlaneDist)
		: AspectRatio(aspectRatio), FOV(fovInDegrees), FarPlaneDistance(farPlaneDist),
		NearPlaneDistance(nearPlaneDist) { }

public:
	void SetFrustumInternals(float aspectRatio, float fovInDegrees, float nearPlaneDist,
		float farPlaneDist)
	{
		AspectRatio = aspectRatio;
		FOV = fovInDegrees;

		FarPlaneDistance = farPlaneDist;
		NearPlaneDistance = nearPlaneDist;

		RecalculateFustrumInternals();
	}

	void CreateFrustum(const Vector3D& cameraPosition, const Vector3D& cameraRight,
		const Vector3D& cameraUp, const Vector3D& cameraForward)
	{
		Vector3D FarPlaneCenter = cameraPosition + cameraForward * FarPlaneDistance;
		Vector3D NearPlaneCenter = cameraPosition + cameraForward * NearPlaneDistance;

		FarPlaneTopLeft = FarPlaneCenter + (cameraUp * FarPlaneHeight * 0.5f) - (cameraRight * FarPlaneWidth * 0.5f);
		FarPlaneTopRight = FarPlaneCenter + (cameraUp * FarPlaneHeight * 0.5f) + (cameraRight * FarPlaneWidth * 0.5f);
		FarPlaneBottomLeft = FarPlaneCenter - (cameraUp * FarPlaneHeight * 0.5f) - (cameraRight * FarPlaneWidth * 0.5f);
		FarPlaneBottomRight = FarPlaneCenter - (cameraUp * FarPlaneHeight * 0.5f) + (cameraRight * FarPlaneWidth * 0.5f);

		NearPlaneTopLeft = NearPlaneCenter + (cameraUp * NearPlaneHeight * 0.5f) - (cameraRight * NearPlaneWidth * 0.5f);
		NearPlaneTopRight = NearPlaneCenter + (cameraUp * NearPlaneHeight * 0.5f) + (cameraRight * NearPlaneWidth * 0.5f);
		NearPlaneBottomLeft = NearPlaneCenter - (cameraUp * NearPlaneHeight * 0.5f) - (cameraRight * NearPlaneWidth * 0.5f);
		NearPlaneBottomRight = NearPlaneCenter - (cameraUp * NearPlaneHeight * 0.5f) + (cameraRight * NearPlaneWidth * 0.5f);

		MakePlaneFromThreePoints(FarPlaneTopRight, FarPlaneTopLeft, FarPlaneBottomLeft, Planes[FARP]);
		MakePlaneFromThreePoints(NearPlaneTopRight, NearPlaneTopLeft, NearPlaneBottomLeft, Planes[NEARP]);
		MakePlaneFromThreePoints(FarPlaneTopRight, FarPlaneTopLeft, NearPlaneTopLeft, Planes[TOP]);
		MakePlaneFromThreePoints(FarPlaneBottomRight, FarPlaneBottomLeft, NearPlaneBottomLeft, Planes[BOTTOM]);
		MakePlaneFromThreePoints(NearPlaneBottomLeft, FarPlaneTopLeft, FarPlaneBottomLeft, Planes[LEFT]);
		MakePlaneFromThreePoints(NearPlaneTopRight, FarPlaneTopRight, FarPlaneBottomRight, Planes[RIGHT]);

	}

	/*
	*  distance -> scales the width of the frustum
	*/
	void CreateFrustum(GW::MATH::GMATRIXF& camModel, float distance, float aspectRatio
		, float nearPlane, float farPlane)
	{
		float y = nearPlane / distance, x = y * aspectRatio;
		Matrix4D Mat = reinterpret_cast<Matrix4D&>(camModel);

		Vector4D NTR = Mat * Vector4D(x, y, nearPlane, 1);
		Vector4D NBR = Mat * Vector4D(x, -y, nearPlane, 1);
		Vector4D NBL = Mat * Vector4D(-x, -y, nearPlane, 1);
		Vector4D NTL = Mat * Vector4D(-x, y, nearPlane, 1);

		y = farPlane / distance; x = y * aspectRatio;
		Vector4D FTR = Mat * Vector4D(x, y, farPlane, 1);
		Vector4D FBR = Mat * Vector4D(x, -y, farPlane, 1);
		Vector4D FBL = Mat * Vector4D(-x, -y, farPlane, 1);
		Vector4D FTL = Mat * Vector4D(-x, y, farPlane, 1);

		FarPlaneTopLeft = Vector3D(FTL.X, FTL.Y, FTL.Z);
		FarPlaneTopRight = Vector3D(FTR.X, FTR.Y, FTR.Z);
		FarPlaneBottomLeft = Vector3D(FBL.X, FBL.Y, FBL.Z);
		FarPlaneBottomRight = Vector3D(FBR.X, FBR.Y, FBR.Z);

		NearPlaneTopLeft = Vector3D(NTL.X, NTL.Y, NTL.Z);
		NearPlaneTopRight = Vector3D(NTR.X, NTR.Y, NTR.Z);
		NearPlaneBottomLeft = Vector3D(NBL.X, NBL.Y, NBL.Z);
		NearPlaneBottomRight = Vector3D(NBR.X, NBR.Y, NBR.Z);

		MakePlaneFromThreePoints(FarPlaneTopRight, FarPlaneTopLeft, FarPlaneBottomLeft, Planes[FARP]);
		MakePlaneFromThreePoints(NearPlaneTopRight, NearPlaneTopLeft, NearPlaneBottomLeft, Planes[NEARP]);
		MakePlaneFromThreePoints(FarPlaneTopRight, FarPlaneTopLeft, NearPlaneTopLeft, Planes[TOP]);
		MakePlaneFromThreePoints(FarPlaneBottomRight, FarPlaneBottomLeft, NearPlaneBottomLeft, Planes[BOTTOM]);
		MakePlaneFromThreePoints(NearPlaneBottomLeft, FarPlaneTopLeft, FarPlaneBottomLeft, Planes[LEFT]);
		MakePlaneFromThreePoints(NearPlaneTopRight, FarPlaneTopRight, FarPlaneBottomRight, Planes[RIGHT]);

		/*GW::MATH::GMATRIXF Inverse = GW::MATH::GIdentityMatrixF;
		GW::MATH::GMatrix::InverseF(camModel, Inverse);
		Mat = reinterpret_cast<Matrix4D&>(Inverse);

		float mx = 1.0F / sqrt(distance * distance + aspectRatio * aspectRatio), my = 1.0F / sqrt(distance * distance + 1.0F);
		Vector4D V1 = Mat * Vector4D(-distance * mx, 0.0F, aspectRatio * mx, 0.0F);
		Vector4D V2 = Mat * Vector4D(0.0F, distance * my, my, 0.0F);
		Vector4D V3 = Mat * Vector4D(distance * mx, 0.0F, aspectRatio * mx, 0.0F);
		Vector4D V4 = Mat * Vector4D(0.0F, -distance * my, my, 0.0F);

		Planes[0] = Plane(V1.X, V1.Y, V1.Z, V1.W);
		Planes[1] = Plane(V2.X, V2.Y, V2.Z, V2.W);
		Planes[2] = Plane(V3.X, V3.Y, V3.Z, V3.W);
		Planes[3] = Plane(V4.X, V4.Y, V4.Z, V4.W);

		float D = 0.0f;

		GW::MATH::GVector::DotF(camModel.row3, camModel.row4, D);
		Vector4D V5 = reinterpret_cast<Vector4D&>(camModel.row3);

		Planes[4] = Plane(V5.X, V5.Y, V5.Z, -(D + nearPlane));
		Planes[5] = Plane(-V5.X, -V5.Y, -V5.Z, D + farPlane);*/
	}

	/*bool SphereVisible(int32 planeCount, const Plane* planeArray,
		const Vector3D& center, float radius)
	{
		float negativeRadius = -radius;
		for (int32 i = 0; i < planeCount; i++)
		{
			if (Dot(planeArray[i], center) <= negativeRadius) return (false);
		}
		return (true);
	}*/

	PlaneIntersectionResult TestAABB(const Vector3D& aabbMin, const Vector3D& aabbMax)
	{
		for (uint32 i = 0; i < 6; ++i)
		{
			if (IntersectAABBOnPlane(aabbMin, aabbMax, Planes[0]) == PlaneIntersectionResult::Back)
			{
				return PlaneIntersectionResult::Back;
			}
		}
	}

	/*--------------------------------------------------DEBUG-------------------------------------------------------*/

	void DebugUpdateVertices(uint32 startVertex, std::vector<Vertex>* vertices)
	{
		Vertex NTL = { {0,0}, {NearPlaneTopLeft}, {0,0,0}, {0.1f,0.1f,0.1f,1} };
		Vertex NTR = { {0,0}, {NearPlaneTopRight}, {0,0,0}, {0.1f,0.1f,0.1f,1} };
		Vertex NBL = { {0,0}, {NearPlaneBottomLeft}, {0,0,0}, {0.1f,0.1f,0.1f,1} };
		Vertex NBR = { {0,0}, {NearPlaneBottomRight}, {0,0,0}, {0.1f,0.1f,0.1f,1} };

		Vertex FTL = { {0,0}, {FarPlaneTopLeft} };
		Vertex FTR = { {0,0}, {FarPlaneTopRight} };
		Vertex FBL = { {0,0}, {FarPlaneBottomLeft} };
		Vertex FBR = { {0,0}, {FarPlaneBottomRight} };

		(*vertices)[startVertex] = NTL;
		(*vertices)[startVertex + 1] = NTR;
		(*vertices)[startVertex + 2] = NBL;
		(*vertices)[startVertex + 3] = NBR;

		(*vertices)[startVertex + 4] = FTL;
		(*vertices)[startVertex + 5] = FTR;
		(*vertices)[startVertex + 6] = FBL;
		(*vertices)[startVertex + 7] = FBR;
	}

	static uint32 GetFrustumIndexCount()
	{
		return 24;
	}

	static uint32 GetFrustumVertexCount()
	{
		return 8;
	}

	static uint32* GetFrustumIndices()
	{
		uint32 Indices[24] =
		{
			0, 1, 1, 3, 3, 2, 2, 0,
			4, 5, 5, 7, 7, 6, 6, 4,
			0, 4, 1, 5, 2, 6, 3, 7
		};

		return Indices;
	}

	/*--------------------------------------------------DEBUG-------------------------------------------------------*/

private:
	void RecalculateFustrumInternals()
	{
		TanFOV = tanf(FOV * 0.5f);

		FarPlaneHeight = TanFOV * FarPlaneDistance;
		NearPlaneHeight = TanFOV * NearPlaneDistance;

		FarPlaneWidth = FarPlaneHeight * AspectRatio;
		NearPlaneWidth = NearPlaneHeight * AspectRatio;
	}

	inline static void MakePlaneFromThreePoints(const Vector3D& a, const Vector3D& b, const Vector3D& c, Plane& outPlane)
	{
		Vector3D EdgeA = b - a;
		Vector3D EdgeB = c - b;
		Vector3D Normal = Vector3D::CrossProduct(EdgeA, EdgeB);
		Normal.Normalize();

		outPlane.X = Normal.X;
		outPlane.Y = Normal.Y;
		outPlane.Z = Normal.Z;

		outPlane.Distance = Vector3D::DotProduct(Normal, a);
	}

	inline static PlaneIntersectionResult IntesectSphereOnPlane(const Vector3D& center, const float radius, const Plane& plane)
	{
		float SphereCenterOffset = Plane::Dot(plane, center);
		float SphereSignedDistance = SphereCenterOffset - plane.Distance;

		//std::cout << SphereSignedDistance << " ";
		if (SphereSignedDistance < 0)
		{
			return PlaneIntersectionResult::Back;
		}
		else if (SphereSignedDistance > 0)
		{
			return PlaneIntersectionResult::Front;
		}
		else
		{
			return PlaneIntersectionResult::Straddling;
		}
	}

	inline static PlaneIntersectionResult IntersectAABBOnPlane(const Vector3D& aabbMin, const Vector3D& aabbMax, const Plane& plane)
	{
		Vector3D SphereCenter = (aabbMin + aabbMax) * 0.5f;
		Vector3D BoxExtents = aabbMax - SphereCenter;
		BoxExtents.X = std::abs(BoxExtents.X);
		BoxExtents.Y = std::abs(BoxExtents.Y);
		BoxExtents.Z = std::abs(BoxExtents.Z);

		float SphereProjectedRadius = Plane::Dot(plane, BoxExtents);

		return IntesectSphereOnPlane(SphereCenter, SphereProjectedRadius, plane);
	}

	//inline bool PointInFrustum(float x, float y, float z)
	//{
	//	for (uint32 i = 0; i < 6; ++i)
	//	{
	//		if (Plane::Dot(Planes[i], Vector3D(x, y, z)) <= 0)
	//		{
	//			// The point was behind a side, so it ISN'T in the frustum
	//			return false;
	//		}
	//	}

	//	// The point was inside of the frustum (In front of ALL the sides of the frustum)
	//	return true;
	//}
};

