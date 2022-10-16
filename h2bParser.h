#ifndef _H2BPARSER_H_
#define _H2BPARSER_H_
#include <fstream>
#include <vector>
#include <set>

namespace H2B {

#pragma pack(push,1)
	struct VECTOR {
		float x, y, z;
	};
	struct VERTEX {
		VECTOR pos, uvw, nrm;
	};
	struct alignas(void*) ATTRIBUTES {
		VECTOR Kd; // diffuse
		float d; // transparency
		VECTOR Ks;  // specular reflectivity
		float Ns;// specular exponent
		VECTOR Ka; // ambient reflectivity
		float sharpness;// local reflection map sharpness
		VECTOR Tf; // transmission filter
		float Ni;// optical density (index of refraction)
		VECTOR Ke; // emissive reflectivity
		unsigned illum;// illumination model
	};
	struct BATCH {
		unsigned indexCount, indexOffset;
	};
#pragma pack(pop)
	struct MATERIAL {
		ATTRIBUTES attrib;
		const char* name;
		const char* map_Kd; // Diffuse map
		const char* map_Ks; // roughness map
		const char* map_Ka; // ambient map
		const char* map_Ke; // emissive map
		const char* map_Ns; // specular 
		const char* map_d; // dissolve map
		const char* disp;
		const char* decal;
		const char* bump; // Normal map
		const void* padding[2];
	};
	struct MESH {
		const char* name;
		BATCH drawInfo;
		unsigned materialIndex;
	};
	class Parser
	{
		std::set<std::string> file_strings;
	public:
		char version[4];
		unsigned vertexCount;
		unsigned indexCount;
		unsigned materialCount;
		unsigned meshCount;
		std::vector<VERTEX> vertices;
		std::vector<unsigned> indices;
		std::vector<MATERIAL> materials;
		std::vector<BATCH> batches;
		std::vector<MESH> meshes;
		bool Parse(const char* h2bPath)
		{
			Clear();
			std::ifstream file;
			char buffer[260] = { 0, };
			file.open(h2bPath, std::ios_base::in |
				std::ios_base::binary);
			if (file.is_open() == false)
				return false;
			file.read(version, 4);
			if (version[1] < '1' || version[2] < '9' || version[3] < 'd')
				return false;
			file.read(reinterpret_cast<char*>(&vertexCount), 4);
			file.read(reinterpret_cast<char*>(&indexCount), 4);
			file.read(reinterpret_cast<char*>(&materialCount), 4);
			file.read(reinterpret_cast<char*>(&meshCount), 4);
			vertices.resize(vertexCount);
			file.read(reinterpret_cast<char*>(vertices.data()), 36 * vertexCount);
			indices.resize(indexCount);
			file.read(reinterpret_cast<char*>(indices.data()), 4 * indexCount);
			materials.resize(materialCount);
			for (int i = 0; i < materialCount; ++i) {
				file.read(reinterpret_cast<char*>(&materials[i].attrib), 80);
				for (int j = 0; j < 10; ++j) {
					buffer[0] = '\0';
					*((&materials[i].name) + j) = nullptr;
					file.getline(buffer, 260, '\0');
					if (buffer[0] != '\0') {
						auto last = file_strings.insert(buffer);
						*((&materials[i].name) + j) = last.first->c_str();
					}
				}
			}
			batches.resize(materialCount);
			file.read(reinterpret_cast<char*>(batches.data()), 8 * materialCount);
			meshes.resize(meshCount);
			for (int i = 0; i < meshCount; ++i) {
				buffer[0] = '\0';
				meshes[i].name = nullptr;
				file.getline(buffer, 260, '\0');
				if (buffer[0] != '\0') {
					auto last = file_strings.insert(buffer);
					meshes[i].name = last.first->c_str();
				}
				file.read(reinterpret_cast<char*>(&meshes[i].drawInfo), 8);
				file.read(reinterpret_cast<char*>(&meshes[i].materialIndex), 4);
			}
			return true;
		}
		void Clear()
		{
			*reinterpret_cast<unsigned*>(version) = 0;
			file_strings.clear();
			vertices.clear();
			indices.clear();
			materials.clear();
			batches.clear();
			meshes.clear();
		}
	};
}
#endif