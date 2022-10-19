#pragma once

#include <vector>
#include "GenericDefines.h"
#include "RawMeshData.h"

/*
* A Collection of Meshes in one
*		IndexOffset -> index offset into the collection of meshes indices
*		MaterialIndex -> Index of the first material into the collection of materials
*		WorldMatrixIndex -> index of the first world matrix into the collection of world matrices
*/
class StaticMesh
{
	/* Rendering information */
private:
	uint32 VertexCount;
	uint32 IndexCount;
	uint32 MaterialCount;
	uint32 MeshCount;
	uint32 InstanceCount;

	uint32 MaterialIndex;
	uint32 WorldMatrixIndex;

	std::vector<Mesh> SubMeshes;

	/* Debug */
	
	bool HasMaterials, HasTextures;
	
	/* Debug */

	/* Per Static Mesh Informations */
private:
	Matrix4D* Transformation;

	/* X, Y, Z translation vector */
	Vector3D Translation;

	/* Pitch, Yaw, Roll rotation vector */
	Vector3D Rotation;
	Vector3D Scale_;

	Vector3D MinBox_AABB;
	Vector3D MaxBox_AABB;

public:
	StaticMesh(bool hasMaterials, bool hasTextures, uint32 vertexCount, uint32 indexCount, uint32 materialCount,
		uint32 materialIndex, uint32 meshCount, uint32 instanceCount, uint32 worldMatrixIndex, Matrix4D* transformMatrix)
		: HasMaterials(hasMaterials), HasTextures(hasTextures), VertexCount(vertexCount), IndexCount(indexCount),	MaterialCount(materialCount), 
		MaterialIndex(materialIndex), MeshCount(meshCount), InstanceCount(instanceCount), 
		WorldMatrixIndex(worldMatrixIndex) 
	{
		Transformation = transformMatrix;

		Vector4D TranslationVector = (*transformMatrix)[3];
		Translation = Vector3D(TranslationVector.X, TranslationVector.Y, TranslationVector.Z);

		Rotation = transformMatrix->GetEulerAngles();
	}

public:
	void Update()
	{
		*Transformation = Matrix4D::MakeRotation(Rotation);
		Transformation->SetTranslation(Translation);
	}

public:
	/* Add a mesh to the model */
	void AddSubMesh(Mesh subMesh)
	{
		SubMeshes.push_back(subMesh);
	}

	/* Translate the model via x, y, z */
	void TranslateLocal(float x, float y, float z)
	{
		Translation.X = x;
		Translation.Y = y;
		Translation.Z = z;

		Transformation->SetTranslation(Translation);
	}

	/* Translate the model via vector3D translation vector */
	void TranslateLocal(Vector3D translation)
	{
		Translation += translation;
		Transformation->SetTranslation(Translation);
	}

	/* Rotate the model with yaw, pitch, roll */
	void Rotate(float yaw, float pitch, float roll)
	{
		Rotation.X += pitch;
		Rotation.Y += yaw;
		Rotation.Z += roll;
	}

	/* Rotate the model with Vector3D -> yaw, pitch, roll */
	void Rotate(Vector3D rotation)
	{
		Rotation += rotation;
	}

	/* Scale the model with x, y, x */
	void Scale(float x, float y, float z)
	{
		Scale_.X += x;
		Scale_.Y += y;
		Scale_.Z += z;
	}

	/* Scale the model with a vector3D -> x, y, z*/
	void Scale(Vector3D scale)
	{
		Scale_ += scale;;
	}

	/*--------------------------------------------------DEBUG-------------------------------------------------------*/

	void SetTransformMatrix(Matrix4D& mat)
	{
		*Transformation = mat;
	}

	Vector4D GetTranslation() const
	{
		return Translation;
	}

	Matrix4D GetTransformMatrix() const
	{
		return *Transformation;
	}

	/*--------------------------------------------------DEBUG-------------------------------------------------------*/

	/* Sub mesh getters */
public:
	uint32 GetSubMeshIndexOffset(uint32 meshIndex) const
	{
		return SubMeshes[meshIndex].IndexOffset;
	}

	uint32 GetSubMeshMaterialIndex(uint32 meshIndex) const
	{
		return SubMeshes[meshIndex].MaterialIndex;
	}

	uint32 GetSubMeshTextureIndex(uint32 meshIndex) const
	{
		return SubMeshes[meshIndex].DiffuseTextureIndex == -1 ? 0 : SubMeshes[meshIndex].DiffuseTextureIndex;
	}

	uint32 GetSubMeshIndexCount(uint32 meshIndex) const
	{
		return SubMeshes[meshIndex].IndexCount;
	}

	/* Getters */
public:
	uint32 GetVertexCount() const
	{
		return VertexCount;
	}

	uint32 GetIndexCount() const
	{
		return IndexCount;
	}

	uint32 GetMaterialCount() const
	{
		return MaterialCount;
	}

	uint32 GetMaterialIndex() const
	{
		return MaterialIndex;
	}

	uint32 GetMeshCount() const
	{
		return MeshCount;
	}

	uint32 GetInstanceCount() const
	{
		return InstanceCount;
	}

	uint32 GetWorldMatrixIndex() const
	{
		return WorldMatrixIndex;
	}

	void SetTextureSupport(bool support)
	{
		HasTextures = support;
	}

	bool SupportsTextures() const
	{
		return HasTextures;
	}

	bool SupportsMaterials() const
	{
		return HasMaterials;
	}
};
