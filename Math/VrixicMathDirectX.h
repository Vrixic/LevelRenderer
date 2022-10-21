#pragma once
#include <DirectXMath.h>
#include "Matrix4D.h"

/* A float4 vector where the X component of the vector is stored in the lowest 32 bits */
typedef DirectX::XMVECTOR VectorRegister;

/* returns and makes a vector with 4 floats */
inline VectorRegister MakeVectorRegister(float x, float y, float z, float w)
{
	return DirectX::XMVectorSet(x, y, z, w);
}

/* returns and makes a vector with 4 floats */
inline VectorRegister MakeVectorRegister(Vector4D v)
{
	return DirectX::XMVectorSet(v.X, v.Y, v.Z, v.W);
}

/* stores a vector register into a Vector4D */
inline void StoreVectorRegister(Vector4D* v, VectorRegister& vectorRegister)
{
	DirectX::XMFLOAT4 v1;
	DirectX::XMStoreFloat4(&v1, vectorRegister);
	v->X = v1.x;
	v->Y = v1.y;
	v->Z = v1.z;
	v->W = v1.w;
}

/* Multiplies two matrices and result is returned via Param1*/
inline void VectorRegisterMatrixMultiply(Matrix4D* Result, const Matrix4D* M1, const Matrix4D* M2)
{
	DirectX::XMMATRIX xMat1 = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)(M1));
	DirectX::XMMATRIX xMat2 = DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)(M2));
	DirectX::XMMATRIX xResult = DirectX::XMMatrixMultiply(xMat1, xMat2);
	DirectX::XMStoreFloat4x4A((DirectX::XMFLOAT4X4A*)(Result), xResult);
}

/* A Homogenous transform */
inline VectorRegister TransformVectorByMatrix(const VectorRegister& V1, const Matrix4D* Transform)
{
	DirectX::XMMATRIX matrix = DirectX::XMLoadFloat4x4A((const DirectX::XMFLOAT4X4A*)(Transform));
	return DirectX::XMVector4Transform(V1, matrix);
}