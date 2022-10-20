#pragma once
#include "GatewareDefine.h"

class Camera
{
	GW::MATH::GMATRIXF ViewMatrix;

	Camera() { }

	Camera(GW::MATH::GMATRIXF& viewMatrix)
	{
		ViewMatrix = viewMatrix;
	}

public:

	void TranslateGlobal(float x, float y, float z)
	{

	}

	void GetViewMatrix()
	{

	}
};
